#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "records.h"
#include "hashfuncs.h"
#include "tagmatrix.h"
#include "devaultInt.h"

#define RD_MAGIC 0x52454353u
#define RD_VERSION 1

typedef struct _record_list_header {
	uint32_t magic;
	uint16_t capacity;
	uint16_t used;
} RecordListHeader;

struct _record_list {
	RecordListHeader hdr;
	struct record *data;
};

bool rd_create_record_table(dv_ctx_t *ctx, uint16_t capacity)
{
	struct record *r = malloc(sizeof(struct record) * capacity);
	if (!r) {
		return false;
	}
	ctx->rl->data = r;
	ctx->rl->hdr.used = 0;
	ctx->rl->hdr.capacity = capacity;
	return true;
}

void rd_destroy_record_table(dv_ctx_t *ctx)
{
	free(ctx->rl->data);
	ctx->rl->data = NULL;
	ctx->rl->hdr.capacity = 0;
}

bool rd_create_record(dv_ctx_t *ctx, const char *name, const char *link)
{
	if (ctx->rl->hdr.used >= ctx->rl->hdr.capacity) {
		uint16_t new_cap = ctx->rl->hdr.capacity * 2;
		if (new_cap == 0)
			new_cap = 4;
		if (new_cap > MAX_RECORDS)
			new_cap = MAX_RECORDS;
		if (ctx->rl->hdr.used >= new_cap)
			return false;

		struct record *tmp =
		    realloc(ctx->rl->data, sizeof(struct record) * new_cap);
		if (!tmp)
			return false;
		ctx->rl->data = tmp;
		ctx->rl->hdr.capacity = new_cap;
	}

	int rec_id = tm_alloc_record(ctx);
	if (rec_id == -1)
		return false;

	strncpy(ctx->rl->data[ctx->rl->hdr.used].name, name, MAX_NAME_LEN - 1);
	ctx->rl->data[ctx->rl->hdr.used].name[MAX_NAME_LEN - 1] = '\0';
	strncpy(ctx->rl->data[ctx->rl->hdr.used].link, link, MAX_URL_LEN - 1);
	ctx->rl->data[ctx->rl->hdr.used].link[MAX_URL_LEN - 1] = '\0';
	ctx->rl->data[ctx->rl->hdr.used].id = (uint16_t)rec_id;
	ctx->rl->data[ctx->rl->hdr.used].hash =
	    fnv1a_64_str(ctx->rl->data[ctx->rl->hdr.used].name);

	ctx->rl->hdr.used++;

	return true;
}

bool rd_delete_record(dv_ctx_t *ctx, const char *name)
{
	uint64_t hash = fnv1a_64_str(name);
	for (uint16_t i = 0; i < ctx->rl->hdr.used; i++) {
		if (ctx->rl->data[i].hash == hash) {
			tm_delete_record(ctx, ctx->rl->data[i].id);
			if (i < ctx->rl->hdr.used - 1) {
				memmove(&ctx->rl->data[i],
					&ctx->rl->data[i + 1],
					sizeof(struct record) *
					    (ctx->rl->hdr.used - i - 1));
			}
			ctx->rl->hdr.used--;
			return true;
		}
	}
	return false;
}

int rd_save(dv_ctx_t *ctx, const char *path)
{
	FILE *f = fopen(path, "wb");
	if (!f)
		return -1;

	uint32_t magic = RD_MAGIC;
	uint8_t version = RD_VERSION;
	fwrite(&magic, sizeof(magic), 1, f);
	fwrite(&version, sizeof(version), 1, f);
	fwrite(&ctx->rl->hdr.capacity, sizeof(ctx->rl->hdr.capacity), 1, f);
	fwrite(&ctx->rl->hdr.used, sizeof(ctx->rl->hdr.used), 1, f);

	fwrite(ctx->rl->data, sizeof(struct record), ctx->rl->hdr.used, f);

	fclose(f);
	return 0;
}

int rd_load(dv_ctx_t *ctx, const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return -1;

	uint32_t magic;
	uint8_t version;
	fread(&magic, sizeof(magic), 1, f);
	fread(&version, sizeof(version), 1, f);

	if (magic != RD_MAGIC) {
		fprintf(stderr, "rd_load: bad magic number\n");
		fclose(f);
		return -1;
	}

	uint16_t capacity;
	uint16_t used;
	fread(&capacity, sizeof(capacity), 1, f);
	fread(&used, sizeof(used), 1, f);

	struct record *data = malloc(sizeof(struct record) * capacity);
	if (!data) {
		fclose(f);
		return -1;
	}

	fread(data, sizeof(struct record), used, f);

	free(ctx->rl->data);
	ctx->rl->data = data;
	ctx->rl->hdr.capacity = capacity;
	ctx->rl->hdr.used = used;

	fclose(f);
	return 0;
}

void rd_print_records(dv_ctx_t ctx)
{
	for (uint16_t i = 0; i < ctx.rl->hdr.used; i++) {
		printf("id=%u  name=%s  link=%s\n",
		       ctx.rl->data[i].id,
		       ctx.rl->data[i].name,
		       ctx.rl->data[i].link);
	}
}
