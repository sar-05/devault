#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "tags.h"
#include "hashfuncs.h"
#include "tagmatrix.h"
#include "devaultInt.h"

#define TG_MAGIC 0x54414753u
#define TG_VERSION 1

typedef struct _tag_list_header {
	uint32_t magic;
	uint16_t capacity;
	uint16_t used;
} TagListHeader;

struct _tag_list {
	TagListHeader hdr;
	struct tag *data;
};

bool tg_create_tag_table(dv_ctx_t *ctx, uint16_t capacity)
{
	struct tag *r = malloc(sizeof(struct tag) * capacity);
	if (!r) {
		return false;
	}
	ctx->tl->data = r;
	ctx->tl->hdr.used = 0;
	ctx->tl->hdr.capacity = capacity;
	return true;
}

void tg_destroy_tag_table(dv_ctx_t *ctx)
{
	free(ctx->tl->data);
	ctx->tl->data = NULL;
	ctx->tl->hdr.capacity = 0;
}

bool tg_create_tag(dv_ctx_t *ctx, const char *name)
{
	if (ctx->tl->hdr.used >= ctx->tl->hdr.capacity) {
		uint16_t new_cap = ctx->tl->hdr.capacity * 2;
		if (new_cap == 0)
			new_cap = 4;
		if (new_cap > MAX_TAGS)
			new_cap = MAX_TAGS;
		if (ctx->tl->hdr.used >= new_cap)
			return false;

		struct tag *tmp =
		    realloc(ctx->tl->data, sizeof(struct tag) * new_cap);
		if (!tmp)
			return false;
		ctx->tl->data = tmp;
		ctx->tl->hdr.capacity = new_cap;
	}

	int tag_id = tm_alloc_tag(ctx);
	if (tag_id == -1)
		return false;

	strncpy(ctx->tl->data[ctx->tl->hdr.used].name, name, MAX_NAME_LEN - 1);
	ctx->tl->data[ctx->tl->hdr.used].name[MAX_NAME_LEN - 1] = '\0';
	ctx->tl->data[ctx->tl->hdr.used].id = (uint16_t)tag_id;
	ctx->tl->data[ctx->tl->hdr.used].hash =
	    fnv1a_64_str(ctx->tl->data[ctx->tl->hdr.used].name);

	ctx->tl->hdr.used++;

	return true;
}

bool tg_delete_tag(dv_ctx_t *ctx, const char *name)
{
	uint64_t hash = fnv1a_64_str(name);
	for (uint16_t i = 0; i < ctx->tl->hdr.used; i++) {
		if (ctx->tl->data[i].hash == hash) {
			tm_delete_tag(ctx, ctx->tl->data[i].id);
			if (i < ctx->tl->hdr.used - 1) {
				memmove(&ctx->tl->data[i],
					&ctx->tl->data[i + 1],
					sizeof(struct tag) *
					    (ctx->tl->hdr.used - i - 1));
			}
			ctx->tl->hdr.used--;
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
	fwrite(&ctx->tl->hdr.capacity, sizeof(ctx->tl->hdr.capacity), 1, f);
	fwrite(&ctx->tl->hdr.used, sizeof(ctx->tl->hdr.used), 1, f);

	fwrite(ctx->tl->data, sizeof(struct tag), ctx->tl->hdr.used, f);

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

	free(ctx->tl->data);
	ctx->tl->data = data;
	ctx->tl->hdr.capacity = capacity;
	ctx->tl->hdr.used = used;

	fclose(f);
	return 0;
}

void tg_print_tags(dv_ctx_t ctx)
{
	for (uint16_t i = 0; i < ctx.tl->hdr.used; i++) {
		printf("id=%u  name=%s\n",
		       ctx.tl->data[i].id,
		       ctx.tl->data[i].name);
	}
}
