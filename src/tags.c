#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tags.h"
#include "devault.h"
#include "hashfuncs.h"
#include "tagmatrix.h"
#include "devaultInt.h"

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
		if (ctx->tag_list->hdr.used >= new_cap) {
			ctx->error_status = ERR_TAG_CREATE_FAILED;
			return false;
		}

		struct tag *tmp =
		    realloc(ctx->tag_list->data, sizeof(struct tag) * new_cap);
		if (!tmp) {
			ctx->error_status = ERR_INTERNAL;
			ctx->error_msg = strerror(errno);
			return false;
		}
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

int tg_write(dv_ctx_t *ctx, FILE *f)
{
	fwrite(&ctx->tag_list->hdr, sizeof(ctx->tag_list->hdr), 1, f);
	fwrite(ctx->tag_list->data,
	       sizeof(struct tag),
	       ctx->tag_list->hdr.used,
	       f);
	return 0;
}

int tg_read(dv_ctx_t *ctx, FILE *f)
{
	TagListHeader hdr;
	if (fread(&hdr, sizeof(hdr), 1, f) != 1)
		return -1;

	struct tag *data = malloc(sizeof(struct tag) * hdr.capacity);
	if (!data)
		return -1;

	if (fread(data, sizeof(struct tag), hdr.used, f) != hdr.used) {
		free(data);
		return -1;
	}

	free(ctx->tag_list->data);
	ctx->tag_list->data = data;
	ctx->tag_list->hdr = hdr;

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

bool tg_is_tag_list_empty(dv_ctx_t *ctx)
{
	return ctx->tag_list->hdr.used == 0;
}
