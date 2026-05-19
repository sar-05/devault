#include <stdio.h>
#include <stdint.h>
#include "devault.h"
#include "devaultInt.h"
#include "records.h"
#include "tagmatrix.h"
#include "spawn.h"
#include "display.h"

struct _dl_ctx {
	dv_ctx_t *ctx;
	const uint16_t *ids;
	uint16_t count;
};

static void _display_list_cb(void *arg)
{
	struct _dl_ctx *dc = arg;
	for (uint16_t i = 0; i < dc->count; i++) {
		const struct record *r =
		    rd_get_record_by_id(dc->ctx, dc->ids[i]);
		if (r)
			printf("[%u] %s  (%s)\n", r->id, r->name, r->link);
	}
}

void display_record_list(dv_ctx_t *ctx, const uint16_t *ids, uint16_t count)
{
	if (count <= 10) {
		for (uint16_t i = 0; i < count; i++) {
			const struct record *r =
			    rd_get_record_by_id(ctx, ids[i]);
			if (r)
				printf(
				    "[%u] %s  (%s)\n", r->id, r->name, r->link);
		}
	} else {
		struct _dl_ctx dc = {ctx, ids, count};
		pipe_to_pager(_display_list_cb, &dc);
	}
}

void display_record_with_tags(dv_ctx_t *ctx, uint16_t record_id)
{
	const struct record *r = rd_get_record_by_id(ctx, record_id);
	if (!r)
		return;

	printf("[%u] %s\n  URL: %s\n  Tags:", r->id, r->name, r->link);
	TagList *tl = ctx->tag_list;
	int count = 0;
	for (uint16_t i = 0; i < tl->hdr.used; i++) {
		if (tm_has_tag(ctx, record_id, tl->data[i].id)) {
			printf(" %s", tl->data[i].name);
			count++;
		}
	}
	if (count == 0)
		printf(" (none)");
	printf("\n");
}

static void _display_all_cb(void *vctx)
{
	dv_ctx_t *ctx = vctx;
	RecordList *rl = ctx->record_list;
	for (uint16_t i = 0; i < rl->hdr.used; i++) {
		display_record_with_tags(ctx, rl->data[i].id);
		printf("\n");
	}
}

void display_all_records(dv_ctx_t *ctx)
{
	pipe_to_pager(_display_all_cb, ctx);
}

void display_separator(FILE *fp)
{
	for (int i = 0; i < SEPARATOR_LEN; i++)
		fputc('-', fp);
	fputc('\n', fp);
}
