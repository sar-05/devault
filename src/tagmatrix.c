#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "devaultInt.h"
#include "devault.h"
#include "tagmatrix.h"

#define MAGIC 0x54414D58u
#define VERSION 1
#define HEADER_SIZE 32

#define BITS_TO_BYTES(n_bits) (((n_bits) + 7) / 8)

#define FL_NONE 0xFFFFu

#define MIN_ROW_STRIDE 2

typedef struct _tag_matrix_header {
	uint32_t magic;
	uint16_t m_capacity;
	uint16_t n_capacity;
	uint16_t m_used;
	uint16_t n_used;
	uint16_t fl_head;
	uint8_t version;
	uint8_t fl_tag_count;
} TagMatrixHeader;

struct _tagmatrix {
	TagMatrixHeader hdr;
	uint8_t _padding[16];

	uint8_t fl_tag_ids[MAX_TAGS];
	uint8_t *data;
	int row_stride;
};

static int _tm_has_tag(const TagMatrix *tm, int R, int T)
{
	return (tm->data[R * tm->row_stride + T / 8] >> (T % 8)) & 1;
}

static void _tm_set_tag(TagMatrix *tm, int R, int T)
{
	tm->data[R * tm->row_stride + T / 8] |= (uint8_t)(1u << (T % 8));
}

static void _tm_clear_tag(TagMatrix *tm, int R, int T)
{
	tm->data[R * tm->row_stride + T / 8] &= (uint8_t)~(1u << (T % 8));
}

int tm_create(dv_ctx_t *ctx, uint16_t m_capacity, uint16_t n_capacity)
{
	TagMatrix *tm = calloc(1, sizeof(TagMatrix));
	if (!tm)
		return -1;

	tm->hdr.magic = MAGIC;
	tm->hdr.version = VERSION;
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

	ctx->tg = tm;
	return 0;
}

void tm_free(dv_ctx_t *ctx)
{
	if (ctx->tg) {
		free(ctx->tg->data);
		free(ctx->tg);
		ctx->tg = NULL;
	}
}

int tm_save(dv_ctx_t *ctx, const char *path)
{
	const TagMatrix *tm = ctx->tg;
	FILE *f = fopen(path, "wb");
	if (!f)
		return -1;

	fwrite(&tm->hdr, sizeof(TagMatrixHeader), 1, f);

	fwrite(tm->fl_tag_ids, sizeof(uint8_t), tm->hdr.fl_tag_count, f);

	size_t bitmap_bytes = (size_t)tm->hdr.m_capacity * tm->row_stride;
	fwrite(tm->data, 1, bitmap_bytes, f);

	fclose(f);
	return 0;
}

int tm_load(dv_ctx_t *ctx, const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return -1;

	TagMatrix *tm = calloc(1, sizeof(TagMatrix));
	if (!tm) {
		fclose(f);
		return -1;
	}

	fread(&tm->hdr, sizeof(TagMatrixHeader), 1, f);

	if (tm->hdr.magic != MAGIC) {
		fprintf(stderr, "tm_load: bad magic number\n");
		free(tm);
		fclose(f);
		return -1;
	}

	fread(tm->fl_tag_ids, sizeof(uint8_t), tm->hdr.fl_tag_count, f);

	int stride = BITS_TO_BYTES(tm->hdr.n_capacity);
	tm->row_stride = stride < MIN_ROW_STRIDE ? MIN_ROW_STRIDE : stride;

	size_t bitmap_bytes = (size_t)tm->hdr.m_capacity * tm->row_stride;
	tm->data = malloc(bitmap_bytes);
	if (!tm->data) {
		free(tm);
		fclose(f);
		return -1;
	}

	fread(tm->data, 1, bitmap_bytes, f);
	fclose(f);
	ctx->tg = tm;
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
	TagMatrix *tm = ctx->tg;
	memset(tm->data + (size_t)R * tm->row_stride, 0, tm->row_stride);
	_fl_set_next(tm, R, tm->hdr.fl_head);
	tm->hdr.fl_head = (uint16_t)R;
}

int tm_alloc_record(dv_ctx_t *ctx)
{
	TagMatrix *tm = ctx->tg;
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
		if (tm->hdr.m_used >= new_cap)
			return -1;
		if (_tm_grow_records(tm, new_cap) != 0)
			return -1;
	}
	return tm->hdr.m_used++;
}

void tm_delete_tag(dv_ctx_t *ctx, int T)
{
	TagMatrix *tm = ctx->tg;
	for (int r = 0; r < tm->hdr.m_used; r++)
		_tm_clear_tag(tm, r, T);

	tm->fl_tag_ids[tm->hdr.fl_tag_count++] = (uint8_t)T;
}

int tm_alloc_tag(dv_ctx_t *ctx)
{
	TagMatrix *tm = ctx->tg;
	if (tm->hdr.fl_tag_count > 0)
		return tm->fl_tag_ids[--tm->hdr.fl_tag_count];

	if (tm->hdr.n_used >= tm->hdr.n_capacity) {
		uint16_t new_cap =
		    tm->hdr.n_capacity + (tm->hdr.n_capacity / 2) + 1;
		if (new_cap > MAX_TAGS)
			new_cap = MAX_TAGS;
		if (tm->hdr.n_used >= new_cap)
			return -1;
		if (_tm_grow_tags(tm, new_cap) != 0)
			return -1;
	}
	return tm->hdr.n_used++;
}

int *tm_query(dv_ctx_t *ctx, const int *tag_ids, int n_tag_ids, int *out_count)
{
	const TagMatrix *tm = ctx->tg;
	uint8_t *query = calloc(1, tm->row_stride);
	if (!query) {
		*out_count = 0;
		return NULL;
	}
	for (int i = 0; i < n_tag_ids; i++)
		query[tag_ids[i] / 8] |= (1u << (tag_ids[i] % 8));

	int *results = malloc((size_t)tm->hdr.m_used * sizeof(int));
	int count = 0;

	for (int r = 0; r < tm->hdr.m_used; r++) {
		uint8_t *row = tm->data + (size_t)r * tm->row_stride;
		int match = 1;
		for (int b = 0; b < tm->row_stride; b++) {
			if ((row[b] & query[b]) != query[b]) {
				match = 0;
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
	const TagMatrix *tm = ctx->tg;
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
			printf("%-4d", _tm_has_tag(tm, r, t));
		printf("\n");
	}
}
