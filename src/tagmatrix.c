#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "devaultInt.h"
#include "devault.h"
#include "tagmatrix.h"

#define BITS_TO_BYTES(n_bits) (((n_bits) + 7) / 8)

#define FL_NONE 0xFFFFu

#define MIN_ROW_STRIDE 2

int tm_has_tag(dv_ctx_t *ctx, uint16_t record_id, uint16_t tag_id)
{
	const TagMatrix *tm = ctx->tag_mat;
	return (tm->data[record_id * tm->row_stride + tag_id / 8] >>
		(tag_id % 8)) &
	       1;
}

void tm_set_tag(dv_ctx_t *ctx, uint16_t record_id, uint16_t tag_id)
{
	TagMatrix *tm = ctx->tag_mat;
	tm->data[record_id * tm->row_stride + tag_id / 8] |=
	    (uint8_t)(1u << (tag_id % 8));
}

void tm_clear_tag(dv_ctx_t *ctx, uint16_t record_id, uint16_t tag_id)
{
	TagMatrix *tm = ctx->tag_mat;
	tm->data[record_id * tm->row_stride + tag_id / 8] &=
	    (uint8_t)~(1u << (tag_id % 8));
}

int tm_create(dv_ctx_t *ctx, uint16_t m_capacity, uint16_t n_capacity)
{
	TagMatrix *tm = calloc(1, sizeof(TagMatrix));
	if (!tm)
		return -1;

	tm->hdr.m_capacity = m_capacity;
	tm->hdr.n_capacity = n_capacity;
	tm->hdr.m_used = 0;
	tm->hdr.n_used = 0;
	tm->hdr.fl_head = FL_NONE;
	tm->hdr.fl_tag_count = 0;

	int stride = BITS_TO_BYTES(n_capacity);
	tm->row_stride = stride < MIN_ROW_STRIDE ? MIN_ROW_STRIDE : stride;

	tm->data = calloc(m_capacity, tm->row_stride);
	if (!tm->data) {
		free(tm);
		return -1;
	}

	ctx->tag_mat = tm;
	return 0;
}

void tm_free(dv_ctx_t *ctx)
{
	if (ctx->tag_mat) {
		free(ctx->tag_mat->data);
		free(ctx->tag_mat);
		ctx->tag_mat = NULL;
	}
}

int tm_write(dv_ctx_t *ctx, FILE *f)
{
	const TagMatrix *tm = ctx->tag_mat;

	fwrite(&tm->hdr, sizeof(tm->hdr), 1, f);
	fwrite(tm->fl_tag_ids, 1, tm->hdr.fl_tag_count, f);

	size_t bitmap_bytes = (size_t)tm->hdr.m_capacity * tm->row_stride;
	fwrite(tm->data, 1, bitmap_bytes, f);
	return 0;
}

int tm_read(dv_ctx_t *ctx, FILE *f)
{
	TagMatrix *tm = calloc(1, sizeof(TagMatrix));
	if (!tm)
		return -1;

	if (fread(&tm->hdr, sizeof(tm->hdr), 1, f) != 1) {
		free(tm);
		return -1;
	}

	if (fread(tm->fl_tag_ids, 1, tm->hdr.fl_tag_count, f) !=
	    tm->hdr.fl_tag_count) {
		free(tm);
		return -1;
	}

	int stride = BITS_TO_BYTES(tm->hdr.n_capacity);
	tm->row_stride = stride < MIN_ROW_STRIDE ? MIN_ROW_STRIDE : stride;

	size_t bitmap_bytes = (size_t)tm->hdr.m_capacity * tm->row_stride;
	tm->data = malloc(bitmap_bytes);
	if (!tm->data) {
		free(tm);
		return -1;
	}

	if (fread(tm->data, 1, bitmap_bytes, f) != bitmap_bytes) {
		free(tm->data);
		free(tm);
		return -1;
	}

	tm_free(ctx);
	ctx->tag_mat = tm;
	return 0;
}

static int _tm_grow_records(TagMatrix *tm, uint16_t new_m_capacity)
{
	if (new_m_capacity <= tm->hdr.m_capacity)
		return 0;
	if (new_m_capacity > MAX_RECORDS)
		return -1;

	size_t new_bytes = (size_t)new_m_capacity * tm->row_stride;
	uint8_t *new_data = realloc(tm->data, new_bytes);
	if (!new_data)
		return -1;

	size_t old_bytes = (size_t)tm->hdr.m_capacity * tm->row_stride;
	memset(new_data + old_bytes, 0, new_bytes - old_bytes);

	tm->data = new_data;
	tm->hdr.m_capacity = new_m_capacity;
	return 0;
}

static int _tm_grow_tags(TagMatrix *tm, uint16_t new_n_capacity)
{
	if (new_n_capacity <= tm->hdr.n_capacity)
		return 0;
	if (new_n_capacity > MAX_TAGS)
		return -1;

	int new_stride = BITS_TO_BYTES(new_n_capacity);
	if (new_stride < MIN_ROW_STRIDE)
		new_stride = MIN_ROW_STRIDE;

	uint8_t *new_data = calloc(tm->hdr.m_capacity, new_stride);
	if (!new_data)
		return -1;

	for (int r = 0; r < tm->hdr.m_capacity; r++) {
		uint8_t *src = tm->data + (size_t)r * tm->row_stride;
		uint8_t *dst = new_data + (size_t)r * new_stride;
		memcpy(dst, src, tm->row_stride);
	}

	free(tm->data);
	tm->data = new_data;
	tm->hdr.n_capacity = new_n_capacity;
	tm->row_stride = new_stride;
	return 0;
}

static uint16_t _fl_get_next(const TagMatrix *tm, int R)
{
	uint16_t next;
	memcpy(&next, tm->data + (size_t)R * tm->row_stride, sizeof(next));
	return next;
}

static void _fl_set_next(TagMatrix *tm, int R, uint16_t next)
{
	memcpy(tm->data + (size_t)R * tm->row_stride, &next, sizeof(next));
}

void tm_delete_record(dv_ctx_t *ctx, int R)
{
	TagMatrix *tm = ctx->tag_mat;
	memset(tm->data + (size_t)R * tm->row_stride, 0, tm->row_stride);
	_fl_set_next(tm, R, tm->hdr.fl_head);
	tm->hdr.fl_head = (uint16_t)R;
}

int tm_alloc_record(dv_ctx_t *ctx)
{
	TagMatrix *tm = ctx->tag_mat;
	if (tm->hdr.fl_head != FL_NONE) {
		int R = tm->hdr.fl_head;
		tm->hdr.fl_head = _fl_get_next(tm, R);
		memset(
		    tm->data + (size_t)R * tm->row_stride, 0, tm->row_stride);
		return R;
	}

	if (tm->hdr.m_used >= tm->hdr.m_capacity) {
		uint16_t new_cap =
		    tm->hdr.m_capacity + (tm->hdr.m_capacity / 2) + 1;
		if (new_cap > MAX_RECORDS)
			new_cap = MAX_RECORDS;
		if (tm->hdr.m_used >= new_cap) {
			ctx->error_status = ERR_RECORD_CREATE_FAILED;
			return -1;
		}
		if (_tm_grow_records(tm, new_cap) != 0) {
			ctx->error_status = ERR_INTERNAL;
			ctx->error_msg = strerror(errno);
			return -1;
		}
	}
	return tm->hdr.m_used++;
}

void tm_delete_tag(dv_ctx_t *ctx, int T)
{
	TagMatrix *tm = ctx->tag_mat;
	for (int r = 0; r < tm->hdr.m_used; r++)
		tm->data[r * tm->row_stride + T / 8] &=
		    (uint8_t)~(1u << (T % 8));

	tm->fl_tag_ids[tm->hdr.fl_tag_count++] = (uint8_t)T;
}

int tm_alloc_tag(dv_ctx_t *ctx)
{
	TagMatrix *tm = ctx->tag_mat;
	if (tm->hdr.fl_tag_count > 0)
		return tm->fl_tag_ids[--tm->hdr.fl_tag_count];

	if (tm->hdr.n_used >= tm->hdr.n_capacity) {
		uint16_t new_cap =
		    tm->hdr.n_capacity + (tm->hdr.n_capacity / 2) + 1;
		if (new_cap > MAX_TAGS)
			new_cap = MAX_TAGS;
		if (tm->hdr.n_used >= new_cap) {
			ctx->error_status = ERR_TAG_CREATE_FAILED;
			return -1;
		}
		if (_tm_grow_tags(tm, new_cap) != 0) {
			ctx->error_status = ERR_INTERNAL;
			ctx->error_msg = strerror(errno);
			return -1;
		}
	}
	return tm->hdr.n_used++;
}

uint16_t *tm_query(dv_ctx_t *ctx,
		   const uint16_t *tag_ids,
		   uint16_t n_tag_ids,
		   uint16_t *out_count)
{
	if (n_tag_ids == 0) {
		*out_count = 0;
		return NULL;
	}

	const TagMatrix *tm = ctx->tag_mat;
	uint8_t *query = calloc(1, tm->row_stride);
	if (!query) {
		*out_count = 0;
		return NULL;
	}
	for (uint16_t i = 0; i < n_tag_ids; i++)
		query[tag_ids[i] / 8] |= (1u << (tag_ids[i] % 8));

	uint16_t *results = malloc((size_t)tm->hdr.m_used * sizeof(uint16_t));
	uint16_t count = 0;

	uint16_t r;
	for (r = 0; r < tm->hdr.m_used; r++) {
		uint8_t *row = tm->data + r * tm->row_stride;
		bool match = true;
		int b;
		for (b = 0; b < tm->row_stride; b++) {
			if ((row[b] & query[b]) != query[b]) {
				match = false;
				break;
			}
		}
		if (match)
			results[count++] = r;
	}

	free(query);
	*out_count = count;
	return results; /* caller must free() */
}

void tm_print(dv_ctx_t *ctx)
{
	const TagMatrix *tm = ctx->tag_mat;
	printf("TagMatrix: records=%d/%d  tags=%d/%d  row_stride=%d bytes\n",
	       tm->hdr.m_used,
	       tm->hdr.m_capacity,
	       tm->hdr.n_used,
	       tm->hdr.n_capacity,
	       tm->row_stride);
	printf("  record free list: fl_head=%d\n", tm->hdr.fl_head);
	printf("  tag free list:    count=%d  ids=[", tm->hdr.fl_tag_count);
	for (int i = 0; i < tm->hdr.fl_tag_count; i++)
		printf("%s%d", i ? "," : "", tm->fl_tag_ids[i]);
	printf("]\n");

	printf("     ");
	for (int t = 0; t < tm->hdr.n_used; t++)
		printf("T%-2d ", t);
	printf("\n");

	for (int r = 0; r < tm->hdr.m_used; r++) {
		printf("R%-3d ", r);
		for (int t = 0; t < tm->hdr.n_used; t++)
			printf("%-4d",
			       (tm->data[r * tm->row_stride + t / 8] >>
				(t % 8)) &
				   1);
		printf("\n");
	}
}
