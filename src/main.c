#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "devault.h"
#include "devaultInt.h"
#include "records.h"
#include "tags.h"
#include "tagmatrix.h"
#include "spawn.h"
#include "validate.h"

static void _list_records_with_tags(void *vctx);

int main(void)
{
	dv_ctx_t *ctx = create_dv_ctx();
	if (!ctx) {
		fprintf(stderr, "Failed to initialize context\n");
		return 1;
	}

	for (;;) {
		for (int i = 0; i < SEPARATOR_LEN; i++)
			fputc('=', stdout);
		fputc('\n', stdout);
		printf("  devault data vault manager\n");
		for (int i = 0; i < SEPARATOR_LEN; i++)
			fputc('-', stdout);
		fputc('\n', stdout);
		printf("  1. Create record\n");
		printf("  2. Delete record\n");
		printf("  3. Tag record\n");
		printf("  4. Remove tag from record\n");
		printf("  5. List records with tags\n");
		printf("  6. List all tags\n");
		printf("  7. Exit\n");
		for (int i = 0; i < SEPARATOR_LEN; i++)
			fputc('-', stdout);
		fputc('\n', stdout);

		int opt = read_menu_opt(ctx, 1, 7);
		if (opt == 7)
			break;

		char name[MAX_NAME_LEN];
		char tag_name[MAX_NAME_LEN];
		char url[MAX_URL_LEN];
		uint16_t record_id;
		uint16_t tag_id;

		switch (opt) {
		case 1:
			read_alphanum_name(ctx, name, sizeof(name));
			read_url(ctx, url, sizeof(url));
			if (rd_create_record(ctx, name, url))
				printf("Record '%s' created.\n", name);
			else
				fprintf(stderr, "Failed to create record.\n");
			break;

		case 2:
			read_alphanum_name(ctx, name, sizeof(name));
			if (rd_delete_record(ctx, name))
				printf("Record '%s' deleted.\n", name);
			else
				fprintf(
				    stderr, "Record '%s' not found.\n", name);
			break;

		case 3:
			read_alphanum_name(ctx, name, sizeof(name));
			record_id = rd_get_record_id_by_name(ctx, name);
			if (record_id == UINT16_MAX) {
				fprintf(
				    stderr, "Record '%s' not found.\n", name);
				break;
			}
			read_alphanum_name(ctx, tag_name, sizeof(tag_name));
			tag_id = tg_get_tag_id_by_name(ctx, tag_name);
			if (tag_id == UINT16_MAX) {
				if (!tg_create_tag(ctx, tag_name)) {
					fprintf(stderr,
						"Failed to create tag.\n");
					break;
				}
				tag_id = tg_get_tag_id_by_name(ctx, tag_name);
			}
			tm_set_tag(ctx, record_id, tag_id);
			printf("Tag '%s' applied to record '%s'.\n",
			       tag_name,
			       name);
			break;

		case 4:
			read_alphanum_name(ctx, name, sizeof(name));
			record_id = rd_get_record_id_by_name(ctx, name);
			if (record_id == UINT16_MAX) {
				fprintf(
				    stderr, "Record '%s' not found.\n", name);
				break;
			}
			read_alphanum_name(ctx, tag_name, sizeof(tag_name));
			tag_id = tg_get_tag_id_by_name(ctx, tag_name);
			if (tag_id == UINT16_MAX) {
				fprintf(
				    stderr, "Tag '%s' not found.\n", tag_name);
				break;
			}
			tm_clear_tag(ctx, record_id, tag_id);
			printf("Tag '%s' removed from record '%s'.\n",
			       tag_name,
			       name);
			break;

		case 5:
			pipe_to_pager(_list_records_with_tags, ctx);
			break;

		case 6:
			tg_print_tags(*ctx);
			break;

		default:
			break;
		}
	}

	destroy_dv_ctx(ctx);
	return 0;
}

static void _list_records_with_tags(void *vctx)
{
	dv_ctx_t *ctx = vctx;
	RecordList *rl = ctx->record_list;
	TagList *tl = ctx->tag_list;

	for (uint16_t i = 0; i < rl->hdr.used; i++) {
		printf("[%u] %s\n  URL: %s\n  Tags:",
		       rl->data[i].id,
		       rl->data[i].name,
		       rl->data[i].link);

		int count = 0;
		for (uint16_t j = 0; j < tl->hdr.used; j++) {
			if (tm_has_tag(ctx, rl->data[i].id, tl->data[j].id)) {
				printf(" %s", tl->data[j].name);
				count++;
			}
		}
		if (count == 0)
			printf(" (none)");
		printf("\n\n");
	}
}
