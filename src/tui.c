#include <regex.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <readline/readline.h>
#include "charts.h"
#include "devault.h"
#include "display.h"
#include "records.h"
#include "tagmatrix.h"
#include "tags.h"
#include "validate.h"
#include "spawn.h"

static void _display_menu(void);
static void _create_record(dv_ctx_t *ctx);
static void _delete_record(dv_ctx_t *ctx);
static void _open_record(dv_ctx_t *ctx);
static void _edit_record_tags(dv_ctx_t *ctx);
static void _show_tags_graphs(dv_ctx_t *ctx);

static uint16_t *_search_records_interactive(dv_ctx_t *ctx, uint16_t *out_count)
{
	fflush(stdout);
	printf("\033[H\033[2J\033[3J");
	display_separator(stdout);
	printf("Record search: \n");
	display_separator(stdout);
	fflush(stdout);
	bool has_tags = tg_print_tags(ctx);
	display_separator(stdout);

	size_t n_tags = 0;
	uint16_t *tag_ids = NULL;

	if (has_tags) {
		tag_ids =
		    read_id_list(ctx, &n_tags, "Enter tag IDs to filter by: ");
		if (!tag_ids) {
			*out_count = 0;
			return NULL;
		}
	} else {
		dv_set_error_status(ctx, ERR_NONE);
	}

	uint16_t *tag_filtered = NULL;
	uint16_t n_tag_filtered = 0;
	if (n_tags > 0) {
		tag_filtered =
		    tm_query(ctx, tag_ids, (uint16_t)n_tags, &n_tag_filtered);
	}
	free(tag_ids);

	char pattern[MAX_REGEX_LEN];
	read_pattern(ctx, pattern, sizeof(pattern));

	uint16_t *matched = NULL;
	if (n_tags > 0 && tag_filtered && n_tag_filtered > 0) {
		matched = rd_search_records_in_set(
		    ctx, pattern, tag_filtered, n_tag_filtered, out_count);
	} else if (n_tags > 0) {
		*out_count = 0;
	} else {
		matched = rd_search_records_by_name(ctx, pattern, out_count);
	}
	free(tag_filtered);

	if (!matched || *out_count == 0) {
		free(matched);
		dv_set_error_status(ctx, ERR_RECORD_SEARCH_NO_MATCH);
		return NULL;
	}

	display_records_by_id(ctx, matched, *out_count);
	return matched;
}

static void _display_menu(void)
{
	printf("\033[H\033[2J\033[3J");
	fflush(stdout);

	display_separator(stdout);
	printf("  devault personal knowledge manager\n");
	display_separator(stdout);

	printf("  1. Create record\n");
	printf("  2. Delete record\n");
	printf("  3. Open record\n");
	printf("  4. Edit record tags\n");
	printf("  5. Show tags graphs\n");
	printf("  6. Exit\n");
	display_separator(stdout);
}

static void _create_record(dv_ctx_t *ctx)
{
	char name[MAX_NAME_LEN];
	char link[MAX_URL_LEN];

	read_input(ctx, name, sizeof(name), TYPE_ALPHANAME);
	if (dv_get_error_status(ctx) != ERR_NONE)
		return;

	read_input(ctx, link, sizeof(link), TYPE_URL);
	if (dv_get_error_status(ctx) != ERR_NONE)
		return;

	if (!rd_create_record(ctx, name, link))
		return;

	dv_save(ctx);
	dv_set_success_msg(ctx, "Record created successfully");
}

static void _delete_record(dv_ctx_t *ctx)
{
	uint16_t count;
	uint16_t *matched = _search_records_interactive(ctx, &count);
	if (!matched)
		return;

	size_t n_sel;
	uint16_t *sel =
	    read_id_list(ctx, &n_sel, "Enter record IDs to delete: ");
	if (!sel) {
		free(matched);
		return;
	}

	for (size_t i = 0; i < n_sel; i++)
		rd_delete_record_by_id(ctx, sel[i]);

	free(sel);
	free(matched);

	dv_save(ctx);
	dv_set_success_msg(ctx, "Record(s) deleted successfully");
}

static void _open_record(dv_ctx_t *ctx)
{
	uint16_t count;
	uint16_t *matched = _search_records_interactive(ctx, &count);
	if (!matched)
		return;

	size_t n_sel;
	uint16_t *sel = read_id_list(ctx, &n_sel, "Enter record IDs to open: ");
	if (!sel) {
		free(matched);
		return;
	}

	for (size_t i = 0; i < n_sel; i++) {
		uint16_t id = sel[i];
		for (;;) {
			fflush(stdout);
			printf("\033[H\033[2J\033[3J");
			display_separator(stdout);
			display_record_by_id(ctx, id);
			display_separator(stdout);
			printf("  1. Open in editor\n");
			printf("  2. Open link in browser\n");
			printf("  3. Exit\n");
			display_separator(stdout);

			int opt = read_menu_opt(ctx, 1, 3);
			if (opt == 3)
				break;

			const struct record *r = rd_get_record_by_id(ctx, id);
			if (!r)
				break;

			switch (opt) {
			case 1: {
				char path[MAX_PATH_LEN];
				snprintf(path,
					 sizeof(path),
					 "%s/%s",
					 dv_get_save_path(ctx),
					 r->name);
				fork_to_editor(path);
				break;
			}
			case 2:
				fork_to_xdg_open(r->link);
				break;
			}
		}
	}

	free(sel);
	free(matched);

	dv_set_success_msg(ctx, "Record(s) opened successfully");
}

static void _edit_record_add_tags(dv_ctx_t *ctx, uint16_t record_id)
{
	printf("Available tags:\n");
	display_all_tags(ctx);
	display_separator(stdout);

	char *line = readline("Enter tag IDs or names: ");
	if (!line)
		return;

	char *tok = strtok(line, " ");
	while (tok) {
		char *end = NULL;
		unsigned long val = strtoul(tok, &end, 10);
		if (end > tok && *end == '\0') {
			uint16_t tag_id = (uint16_t)val;
			if (tg_get_tag_name_by_id(ctx, tag_id))
				tm_set_tag(ctx, record_id, tag_id);
		} else {
			uint16_t tag_id = tg_get_tag_id_by_name(ctx, tok);
			if (tag_id == UINT16_MAX) {
				tg_create_tag(ctx, tok);
				tag_id = tg_get_tag_id_by_name(ctx, tok);
			}
			if (tag_id != UINT16_MAX)
				tm_set_tag(ctx, record_id, tag_id);
		}
		tok = strtok(NULL, " ");
	}
	free(line);
}

static void _edit_record_remove_tags(dv_ctx_t *ctx, uint16_t record_id)
{
	printf("Tags of current record:\n");
	display_record_tags(ctx, record_id);
	display_separator(stdout);

	size_t n_ids;
	uint16_t *ids = read_id_list(ctx, &n_ids, "Enter tag IDs to remove: ");
	if (!ids)
		return;

	for (size_t i = 0; i < n_ids; i++)
		tm_clear_tag(ctx, record_id, ids[i]);

	free(ids);
}

static void _edit_record_tags(dv_ctx_t *ctx)
{
	uint16_t count;
	uint16_t *matched = _search_records_interactive(ctx, &count);
	if (!matched)
		return;

	size_t n_sel;
	uint16_t *sel = read_id_list(ctx, &n_sel, "Enter record IDs to edit: ");
	if (!sel) {
		free(matched);
		return;
	}

	for (size_t i = 0; i < n_sel; i++) {
		uint16_t id = sel[i];
		for (;;) {
			display_record_by_id(ctx, id);
			display_separator(stdout);
			printf("  1. Add tags\n");
			printf("  2. Remove tags\n");
			printf("  3. Done\n");
			display_separator(stdout);

			int opt = read_menu_opt(ctx, 1, 3);
			if (opt == 1)
				_edit_record_add_tags(ctx, id);
			else if (opt == 2)
				_edit_record_remove_tags(ctx, id);
			else
				break;
		}
	}

	free(sel);
	free(matched);

	dv_save(ctx);
	dv_set_success_msg(ctx, "Record tags updated successfully");
}

static void _show_tags_graphs(dv_ctx_t *ctx)
{
	printf("\033[H\033[2J\033[3J");
	fflush(stdout);
	display_separator(stdout);
	printf("  TAG STATISTICS CHARTS\n");
	display_separator(stdout);
	printf("  1. Pie chart\n");
	printf("  2. Bar chart\n");
	printf("  3. Exit\n");
	display_separator(stdout);

	int opt = read_menu_opt(ctx, 1, 3);
	if (opt == 3) {
		dv_set_success_msg(ctx, "Cancelled");
		return;
	}

	if (opt == 1)
		pipe_to_pager(display_pie_chart, ctx);
	else
		pipe_to_pager(display_bar_chart, ctx);

	dv_set_success_msg(ctx, "Chart displayed in pager");
}

void tui_run(dv_ctx_t *ctx)
{
	_display_menu();

	for (;;) {
		int opt = read_menu_opt(ctx, 1, 6);

		if (opt == 6)
			break;

		switch (opt) {
		case 1:
			_create_record(ctx);
			break;
		case 2:
			_delete_record(ctx);
			break;
		case 3:
			_open_record(ctx);
			break;
		case 4:
			_edit_record_tags(ctx);
			break;
		case 5:
			_show_tags_graphs(ctx);
			break;
		default:
			dv_set_error_status(ctx, ERR_OPTION_OUT_OF_RANGE);
		}

		_display_menu();

		dv_error e;
		if ((e = dv_get_error_status(ctx)) != ERR_NONE)
			dv_print_error(ctx);
		else
			dv_print_success(ctx);
	}
}
