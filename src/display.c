#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include "charts.h"
#include "devault.h"
#include "devaultInt.h"
#include "records.h"
#include "tagmatrix.h"
#include "tags.h"
#include "display.h"

void display_tag(dv_ctx_t *ctx, const struct tag *t)
{
	(void)ctx;
	printf("id=%u  name=%s\n", t->id, t->name);
}

void display_tag_by_id(dv_ctx_t *ctx, uint16_t id)
{
	TagList *tl = ctx->tag_list;
	for (uint16_t i = 0; i < tl->hdr.used; i++) {
		if (tl->data[i].id == id) {
			display_tag(ctx, &tl->data[i]);
			return;
		}
	}
}

void display_tags_by_id(dv_ctx_t *ctx, const uint16_t *ids, uint16_t count)
{
	for (uint16_t i = 0; i < count; i++)
		display_tag_by_id(ctx, ids[i]);
}

void display_all_tags(dv_ctx_t *ctx)
{
	TagList *tl = ctx->tag_list;
	for (uint16_t i = 0; i < tl->hdr.used; i++)
		display_tag(ctx, &tl->data[i]);
}

void display_record_tags(dv_ctx_t *ctx, uint16_t record_id)
{
	TagList *tl = ctx->tag_list;
	for (uint16_t i = 0; i < tl->hdr.used; i++) {
		if (tm_has_tag(ctx, record_id, tl->data[i].id))
			display_tag(ctx, &tl->data[i]);
	}
}

void display_separator(FILE *fp)
{
	for (int i = 0; i < SEPARATOR_LEN; i++)
		fputc('-', fp);
	fputc('\n', fp);
}

void display_record(dv_ctx_t *ctx, const struct record *r)
{
	printf("[%u] %s\n  URL: %s\n  Tags:", r->id, r->name, r->link);
	TagList *tl = ctx->tag_list;
	int count = 0;
	for (uint16_t i = 0; i < tl->hdr.used; i++) {
		if (tm_has_tag(ctx, r->id, tl->data[i].id)) {
			printf(" %s", tl->data[i].name);
			count++;
		}
	}
	if (count == 0)
		printf(" (none)");
	printf("\n");
}

void display_record_by_id(dv_ctx_t *ctx, uint16_t id)
{
	const struct record *r = rd_get_record_by_id(ctx, id);
	if (r)
		display_record(ctx, r);
}

void display_records_by_id(dv_ctx_t *ctx, const uint16_t *ids, uint16_t count)
{
	for (uint16_t i = 0; i < count; i++) {
		const struct record *r = rd_get_record_by_id(ctx, ids[i]);
		if (r)
			display_record(ctx, r);
	}
}

void display_all_records(dv_ctx_t *ctx)
{
	RecordList *rl = ctx->record_list;
	for (uint16_t i = 0; i < rl->hdr.used; i++) {
		display_record(ctx, &rl->data[i]);
		printf("\n");
	}
}

static void _count_tags_per_record(dv_ctx_t *ctx, int **out_counts,
				    char ***out_names, int *out_len)
{
	RecordList *rl = ctx->record_list;
	TagList *tl = ctx->tag_list;

	*out_len = (int)tl->hdr.used;
	*out_counts = malloc(*out_len * sizeof(int));
	*out_names = malloc(*out_len * sizeof(char *));

	if (!*out_counts || !*out_names) {
		free(*out_counts);
		free(*out_names);
		*out_counts = NULL;
		*out_names = NULL;
		*out_len = 0;
		return;
	}

	for (int i = 0; i < *out_len; i++) {
		uint16_t tag_id = tl->data[i].id;
		int count = 0;
		for (uint16_t j = 0; j < rl->hdr.used; j++) {
			if (tm_has_tag(ctx, rl->data[j].id, tag_id))
				count++;
		}
		(*out_counts)[i] = count;
		(*out_names)[i] = tl->data[i].name;
	}
}

void display_pie_chart(dv_ctx_t *ctx)
{
	int *counts;
	char **names;
	int len;

	_count_tags_per_record(ctx, &counts, &names, &len);
	if (!counts)
		return;

	print_pie_chart(counts, names, len);

	free(counts);
	free(names);
}

void display_bar_chart(dv_ctx_t *ctx)
{
	int *counts;
	char **names;
	int len;

	_count_tags_per_record(ctx, &counts, &names, &len);
	if (!counts)
		return;

	print_bar_chart(counts, names, len);

	free(counts);
	free(names);
}
