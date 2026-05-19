#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <regex.h>
#include "records.h"
#include "hashfuncs.h"
#include "tagmatrix.h"
#include "devaultInt.h"

bool rd_create_record_list(dv_ctx_t *ctx, uint16_t capacity)
{
	RecordList *rl = malloc(sizeof(RecordList));
	if (!rl)
		return false;

	struct record *d = malloc(sizeof(struct record) * capacity);
	if (!d) {
		free(rl);
		return false;
	}

	ctx->record_list = rl;
	ctx->record_list->data = d;
	ctx->record_list->hdr.used = 0;
	ctx->record_list->hdr.capacity = capacity;
	return true;
}

void rd_destroy_record_list(dv_ctx_t *ctx)
{
	if (!ctx->record_list)
		return;
	free(ctx->record_list->data);
	free(ctx->record_list);
	ctx->record_list = NULL;
}

bool rd_create_record(dv_ctx_t *ctx, const char *name, const char *link)
{
	if (rd_get_record_id_by_name(ctx, name) != UINT16_MAX) {
		ctx->error_status = ERR_USED_RECORD_NAME;
		return false;
	}

	if (ctx->record_list->hdr.used >= ctx->record_list->hdr.capacity) {
		uint16_t new_cap = ctx->record_list->hdr.capacity * 2;
		if (new_cap == 0)
			new_cap = 4;
		if (new_cap > MAX_RECORDS)
			new_cap = MAX_RECORDS;
		if (ctx->record_list->hdr.used >= new_cap) {
			ctx->error_status = ERR_RECORD_CREATE_FAILED;
			return false;
		}

		struct record *tmp = realloc(ctx->record_list->data,
					     sizeof(struct record) * new_cap);
		if (!tmp) {
			ctx->error_status = ERR_INTERNAL;
			ctx->error_msg = strerror(errno);
			return false;
		}
		ctx->record_list->data = tmp;
		ctx->record_list->hdr.capacity = new_cap;
	}

	int rec_id = tm_alloc_record(ctx);
	if (rec_id == -1) {
		ctx->error_status = ERR_RECORD_CREATE_FAILED;
		return false;
	}

	struct record *rec =
	    &ctx->record_list->data[ctx->record_list->hdr.used];

	strncpy(rec->name, name, MAX_NAME_LEN - 1);
	rec->name[MAX_NAME_LEN - 1] = '\0';

	strncpy(rec->link, link, MAX_URL_LEN - 1);
	rec->link[MAX_URL_LEN - 1] = '\0';
	rec->id = (uint16_t)rec_id;
	rec->hash = fnv1a_64_str(rec->name);

	char fpath[MAX_PATH_LEN + MAX_NAME_LEN + 2];
	snprintf(fpath, sizeof(fpath), "%s/%s", ctx->save_path, rec->name);

	FILE *check = fopen(fpath, "r");
	if (check) {
		fclose(check);
		tm_delete_record(ctx, (uint16_t)rec_id);
		ctx->error_status = ERR_FILE_EXISTS;
		return false;
	}

	FILE *rec_file = fopen(fpath, "w");
	if (!rec_file) {
		tm_delete_record(ctx, (uint16_t)rec_id);
		ctx->error_status = ERR_RECORD_CREATE_FAILED;
		return false;
	}
	fclose(rec_file);

	ctx->record_list->hdr.used++;

	return true;
}

bool rd_delete_record_by_id(dv_ctx_t *ctx, uint16_t id)
{
	for (uint16_t i = 0; i < ctx->record_list->hdr.used; i++) {
		if (ctx->record_list->data[i].id == id) {
			tm_delete_record(ctx, id);

			char fpath[MAX_PATH_LEN + MAX_NAME_LEN + 2];
			snprintf(fpath,
				 sizeof(fpath),
				 "%s/%s",
				 ctx->save_path,
				 ctx->record_list->data[i].name);
			remove(fpath);

			if (i < ctx->record_list->hdr.used - 1) {
				memmove(
				    &ctx->record_list->data[i],
				    &ctx->record_list->data[i + 1],
				    sizeof(struct record) *
					(ctx->record_list->hdr.used - i - 1));
			}
			ctx->record_list->hdr.used--;
			return true;
		}
	}
	return false;
}

int rd_write(dv_ctx_t *ctx, FILE *f)
{
	fwrite(&ctx->record_list->hdr, sizeof(ctx->record_list->hdr), 1, f);
	fwrite(ctx->record_list->data,
	       sizeof(struct record),
	       ctx->record_list->hdr.used,
	       f);
	return 0;
}

int rd_read(dv_ctx_t *ctx, FILE *f)
{
	RecordListHeader hdr;
	if (fread(&hdr, sizeof(hdr), 1, f) != 1)
		return -1;

	struct record *data = malloc(sizeof(struct record) * hdr.capacity);
	if (!data)
		return -1;

	if (fread(data, sizeof(struct record), hdr.used, f) != hdr.used) {
		free(data);
		return -1;
	}

	free(ctx->record_list->data);
	ctx->record_list->data = data;
	ctx->record_list->hdr = hdr;

	return 0;
}

void rd_print_records(dv_ctx_t ctx)
{
	for (uint16_t i = 0; i < ctx.record_list->hdr.used; i++) {
		printf("id=%u  name=%s  link=%s\n",
		       ctx.record_list->data[i].id,
		       ctx.record_list->data[i].name,
		       ctx.record_list->data[i].link);
	}
}

const struct record *rd_get_record_by_id(dv_ctx_t *ctx, uint16_t id)
{
	const uint16_t used = ctx->record_list->hdr.used;
	const struct record *data = ctx->record_list->data;

	for (uint16_t i = 0; i < used; i++) {
		if (data[i].id == id)
			return &data[i];
	}

	return NULL;
}

uint16_t rd_get_record_id_by_name(dv_ctx_t *ctx, const char *name)
{
	uint64_t hash = fnv1a_64_str(name);
	for (uint16_t i = 0; i < ctx->record_list->hdr.used; i++) {
		if (ctx->record_list->data[i].hash == hash)
			return ctx->record_list->data[i].id;
	}
	return UINT16_MAX;
}

uint16_t *rd_search_records_by_name(dv_ctx_t *ctx,
				    const char *pattern,
				    uint16_t *out_count)
{
	regex_t re;
	if (regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB) != 0) {
		*out_count = 0;
		return NULL;
	}

	uint16_t *results =
	    malloc(sizeof(uint16_t) * ctx->record_list->hdr.used);
	if (!results) {
		regfree(&re);
		*out_count = 0;
		return NULL;
	}

	uint16_t count = 0;
	for (uint16_t i = 0; i < ctx->record_list->hdr.used; i++) {
		if (regexec(&re, ctx->record_list->data[i].name, 0, NULL, 0) ==
		    0)
			results[count++] = ctx->record_list->data[i].id;
	}

	regfree(&re);
	*out_count = count;
	return results;
}

uint16_t *rd_search_records_in_set(dv_ctx_t *ctx,
				   const char *pattern,
				   const uint16_t *ids,
				   uint16_t ids_count,
				   uint16_t *out_count)
{
	regex_t re;
	if (regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB) != 0) {
		*out_count = 0;
		return NULL;
	}

	bool wanted[MAX_RECORDS] = {false};
	for (uint16_t i = 0; i < ids_count; i++)
		if (ids[i] < MAX_RECORDS)
			wanted[ids[i]] = true;

	uint16_t *results =
	    malloc(sizeof(uint16_t) * ctx->record_list->hdr.used);
	if (!results) {
		regfree(&re);
		*out_count = 0;
		return NULL;
	}

	uint16_t count = 0;
	for (uint16_t i = 0; i < ctx->record_list->hdr.used; i++) {
		uint16_t rid = ctx->record_list->data[i].id;
		if (rid < MAX_RECORDS && wanted[rid] &&
		    regexec(&re, ctx->record_list->data[i].name, 0, NULL, 0) ==
			0)
			results[count++] = rid;
	}

	regfree(&re);
	*out_count = count;
	return results;
}

bool rd_record_has_tags(dv_ctx_t *ctx, uint16_t record_id)
{
	TagList *tl = ctx->tag_list;
	for (uint16_t i = 0; i < tl->hdr.used; i++) {
		if (tm_has_tag(ctx, record_id, tl->data[i].id))
			return true;
	}
	return false;
}

bool rd_is_record_list_empty(dv_ctx_t *ctx)
{
	if (ctx->record_list->hdr.used == 0) {
		ctx->error_status = ERR_EMPTY_RECORD_LIST;
		return true;
	}
	return false;
}
