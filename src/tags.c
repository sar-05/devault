#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tags.h"
#include "devault.h"
#include "hashfuncs.h"
#include "tagmatrix.h"
#include "devaultInt.h"

#define TG_MAGIC 0x54414753u
#define TG_VERSION 1

bool tg_create_tag_table(dv_ctx_t *ctx, uint16_t capacity)
{
	ctx->tag_list = malloc(sizeof(TagList));
	if (!ctx->tag_list)
		return false;

	ctx->tag_list->data = malloc(sizeof(struct tag) * capacity);
	if (!ctx->tag_list->data) {
		free(ctx->tag_list);
		ctx->tag_list = NULL;
		return false;
	}

	ctx->tag_list->hdr.magic = TG_MAGIC;
	ctx->tag_list->hdr.used = 0;
	ctx->tag_list->hdr.capacity = capacity;
	return true;
}

void tg_destroy_tag_table(dv_ctx_t *ctx)
{
	if (!ctx->tag_list)
		return;
	free(ctx->tag_list->data);
	free(ctx->tag_list);
	ctx->tag_list = NULL;
}

bool tg_create_tag(dv_ctx_t *ctx, const char *name)
{
	if (ctx->tag_list->hdr.used >= ctx->tag_list->hdr.capacity) {
		uint16_t new_cap = ctx->tag_list->hdr.capacity * 2;
		if (new_cap == 0)
			new_cap = 4;
		if (new_cap > MAX_TAGS)
			new_cap = MAX_TAGS;
		if (ctx->tag_list->hdr.used >= new_cap)
			return false;

		struct tag *tmp =
		    realloc(ctx->tag_list->data, sizeof(struct tag) * new_cap);
		if (!tmp)
			return false;
		ctx->tag_list->data = tmp;
		ctx->tag_list->hdr.capacity = new_cap;
	}

	int tag_id = tm_alloc_tag(ctx);
	if (tag_id == -1)
		return false;

	strncpy(ctx->tag_list->data[ctx->tag_list->hdr.used].name,
		name,
		MAX_NAME_LEN - 1);
	ctx->tag_list->data[ctx->tag_list->hdr.used].name[MAX_NAME_LEN - 1] =
	    '\0';
	ctx->tag_list->data[ctx->tag_list->hdr.used].id = (uint16_t)tag_id;
	ctx->tag_list->data[ctx->tag_list->hdr.used].hash =
	    fnv1a_64_str(ctx->tag_list->data[ctx->tag_list->hdr.used].name);

	ctx->tag_list->hdr.used++;

	return true;
}

bool tg_delete_tag(dv_ctx_t *ctx, const char *name)
{
	uint64_t hash = fnv1a_64_str(name);
	for (uint16_t i = 0; i < ctx->tag_list->hdr.used; i++) {
		if (ctx->tag_list->data[i].hash == hash) {
			tm_delete_tag(ctx, ctx->tag_list->data[i].id);
			if (i < ctx->tag_list->hdr.used - 1) {
				memmove(&ctx->tag_list->data[i],
					&ctx->tag_list->data[i + 1],
					sizeof(struct tag) *
					    (ctx->tag_list->hdr.used - i - 1));
			}
			ctx->tag_list->hdr.used--;
			return true;
		}
	}
	return false;
}

int tg_save(dv_ctx_t *ctx, const char *path)
{
	FILE *f = fopen(path, "wb");
	if (!f)
		return -1;

	uint32_t magic = TG_MAGIC;
	uint8_t version = TG_VERSION;
	fwrite(&magic, sizeof(magic), 1, f);
	fwrite(&version, sizeof(version), 1, f);
	fwrite(&ctx->tag_list->hdr.capacity,
	       sizeof(ctx->tag_list->hdr.capacity),
	       1,
	       f);
	fwrite(&ctx->tag_list->hdr.used, sizeof(ctx->tag_list->hdr.used), 1, f);

	fwrite(ctx->tag_list->data,
	       sizeof(struct tag),
	       ctx->tag_list->hdr.used,
	       f);

	fclose(f);
	return 0;
}

int tg_load(dv_ctx_t *ctx, const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return -1;

	uint32_t magic;
	uint8_t version;
	fread(&magic, sizeof(magic), 1, f);
	fread(&version, sizeof(version), 1, f);

	if (magic != TG_MAGIC) {
		fprintf(stderr, "tg_load: bad magic number\n");
		fclose(f);
		return -1;
	}

	uint16_t capacity;
	uint16_t used;
	fread(&capacity, sizeof(capacity), 1, f);
	fread(&used, sizeof(used), 1, f);

	struct tag *data = malloc(sizeof(struct tag) * capacity);
	if (!data) {
		fclose(f);
		return -1;
	}

	fread(data, sizeof(struct tag), used, f);

	free(ctx->tag_list->data);
	ctx->tag_list->data = data;
	ctx->tag_list->hdr.capacity = capacity;
	ctx->tag_list->hdr.used = used;

	fclose(f);
	return 0;
}

bool tg_print_tags(dv_ctx_t *ctx)
{
	if (ctx->tag_list->hdr.used == 0) {
		ctx->error_status = ERR_EMPTY_TAG_LIST;
		return false;
	}

	for (uint16_t i = 0; i < ctx->tag_list->hdr.used; i++) {
		printf("id=%u  name=%s\n",
		       ctx->tag_list->data[i].id,
		       ctx->tag_list->data[i].name);
	}
	ctx->error_status = ERR_NONE;
	return true;
}

const char *tg_get_tag_name_by_id(dv_ctx_t *ctx, uint16_t id)
{
	for (uint16_t i = 0; i < ctx->tag_list->hdr.used; i++) {
		if (ctx->tag_list->data[i].id == id)
			return ctx->tag_list->data[i].name;
	}
	return NULL;
}

uint16_t tg_get_tag_id_by_name(dv_ctx_t *ctx, const char *name)
{
	const uint16_t used = ctx->tag_list->hdr.used;
	const struct tag *data = ctx->tag_list->data;

	for (uint16_t i = 0; i < used; i++) {
		if (strcmp(name, data[i].name) == 0)
			return data[i].id;
	}

	return UINT16_MAX;
}

uint16_t *tg_filter_records_by_id(dv_ctx_t *ctx,
				  const char **names,
				  uint16_t len,
				  uint16_t *record_count)
{
	if (len == 0) {
		*record_count = 0;
		return NULL;
	}

	uint16_t count = 0;
	uint16_t *tag_ids = malloc(sizeof(uint16_t) * len);

	if (!tag_ids) {
		*record_count = 0;
		return NULL;
	}

	for (uint16_t i = 0; i < len; i++) {
		uint16_t id = tg_get_tag_id_by_name(ctx, names[i]);
		if (id != UINT16_MAX)
			tag_ids[count++] = id;
	}

	uint16_t out_count;
	uint16_t *record_ids;
	record_ids = tm_query(ctx, tag_ids, count, &out_count);
	free(tag_ids);

	if (out_count == 0) {
		*record_count = 0;
		return NULL;
	}

	*record_count = out_count;
	return record_ids; /* caller must free() */
}
