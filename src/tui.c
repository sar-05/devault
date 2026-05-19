#include <assert.h>
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
#include "display.h"
#include "tui.h"

static const struct record *_select_record_by_search(dv_ctx_t *ctx);

void tui_run(dv_ctx_t *ctx)
{
	printf("\033[H\033[2J\033[3J");
	for (;;) {
		display_separator(stdout);
		printf("  devault personal knowledge manager\n");
		display_separator(stdout);

		printf("  1. Create record\n");
		printf("  2. Delete record\n");
		printf("  3. Tag record\n");
		printf("  4. Remove tag from record\n");
		printf("  5. List records\n");
		printf("  6. List all tags\n");
		printf("  7. Search records\n");
		printf("  8. Exit\n");
		display_separator(stdout);

		int opt = read_menu_opt(ctx, 1, 8);
		if (opt == 8) {
			printf("\033[H\033[2J\033[3J");
			break;
		}

		ctx->error_status = ERR_NONE;

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
				ctx->error_status = ERR_RECORD_CREATE_FAILED;
			break;

		case 2: {
			if (ctx->record_list->hdr.used == 0) {
				ctx->error_status = ERR_EMPTY_RECORD_LIST;
				break;
			}

			const struct record *r = _select_record_by_search(ctx);
			if (!r)
				break;

			char prompt[MAX_NAME_LEN + MAX_URL_LEN + 32];
			snprintf(prompt, sizeof(prompt),
				 "Delete '%s' (%s)? (y/N): ", r->name, r->link);
			char *yn = read_input(prompt);
			if (!yn)
				break;
			int ync = yn[0];
			free(yn);
			if (ync != 'y' && ync != 'Y')
				break;

			rd_delete_record(ctx, r->name);
			printf("Record '%s' deleted.\n", r->name);
			break;
		}

		case 3: {
			if (ctx->record_list->hdr.used == 0) {
				ctx->error_status = ERR_EMPTY_RECORD_LIST;
				break;
			}

			const struct record *r = _select_record_by_search(ctx);
			if (!r)
				break;
			record_id = r->id;

			char *line;
			if (ctx->tag_list->hdr.used > 0) {
				tg_print_tags(ctx);
				line = read_input(
				    "Enter tag ID or new tag name: ");
			} else {
				line = read_input(
				    "No tags yet. Enter a new tag name: ");
			}
			if (!line || line[0] == '\0') {
				free(line);
				break;
			}
			strncpy(tag_name, line, sizeof(tag_name) - 1);
			tag_name[sizeof(tag_name) - 1] = '\0';
			free(line);
				break;

			int parsed;
			if (is_valid_menu_opt(
				ctx, tag_name, 0, MAX_TAGS - 1, &parsed)) {
				tag_id = (uint16_t)parsed;
				if (tg_get_tag_name_by_id(ctx, tag_id) ==
				    NULL) {
					ctx->error_status =
					    ERR_TAG_ID_NOT_FOUND;
					break;
				}
			} else if (ctx->error_status ==
				   ERR_OPTION_NOT_NUMERIC) {
				if (!is_valid_alphanum_name(ctx, tag_name))
					break;
				tag_id = tg_get_tag_id_by_name(ctx, tag_name);
				if (tag_id == UINT16_MAX) {
					if (!tg_create_tag(ctx, tag_name)) {
						ctx->error_status =
						    ERR_TAG_CREATE_FAILED;
						break;
					}
					tag_id = tg_get_tag_id_by_name(
					    ctx, tag_name);
				}
			} else {
				break;
			}

			tm_set_tag(ctx, record_id, tag_id);
			printf("Tag '%s' applied to record '%s'.\n",
			       tag_name,
			       r->name);
			break;
		}

		case 4: {
			if (ctx->record_list->hdr.used == 0) {
				ctx->error_status = ERR_EMPTY_RECORD_LIST;
				break;
			}

			const struct record *r = _select_record_by_search(ctx);
			if (!r)
				break;
			record_id = r->id;

			if (!rd_record_has_tags(ctx, record_id)) {
				ctx->error_status = ERR_NO_TAGS_IN_RECORD;
				break;
			}
			if (!tg_print_tags(ctx))
				break;
			char *line = read_input("Enter tag ID or name: ");
			if (!line || line[0] == '\0') {
				free(line);
				break;
			}
			strncpy(tag_name, line, sizeof(tag_name) - 1);
			tag_name[sizeof(tag_name) - 1] = '\0';
			free(line);
				break;
			int parsed;
			if (is_valid_menu_opt(
				ctx, tag_name, 0, MAX_TAGS - 1, &parsed)) {
				tag_id = (uint16_t)parsed;
				if (tg_get_tag_name_by_id(ctx, tag_id) ==
				    NULL) {
					ctx->error_status =
					    ERR_TAG_ID_NOT_FOUND;
					break;
				}
			} else if (ctx->error_status ==
				   ERR_OPTION_NOT_NUMERIC) {
				if (!is_valid_alphanum_name(ctx, tag_name))
					break;
				tag_id = tg_get_tag_id_by_name(ctx, tag_name);
				if (tag_id == UINT16_MAX) {
					ctx->error_status = ERR_TAG_NOT_FOUND;
					break;
				}
			} else {
				break;
			}
			tm_clear_tag(ctx, record_id, tag_id);
			{
				const char *tn =
				    tg_get_tag_name_by_id(ctx, tag_id);
				assert(tn != NULL);
				printf("Tag '%s' removed from record '%s'.\n",
				       tn,
				       r->name);
			}
			break;
		}

		case 5:
			if (ctx->record_list->hdr.used == 0) {
				ctx->error_status = ERR_EMPTY_RECORD_LIST;
				break;
			}
			display_all_records(ctx);
			break;

		case 6:
			tg_print_tags(ctx);
			break;

		case 7: {
			if (ctx->record_list->hdr.used == 0) {
				ctx->error_status = ERR_EMPTY_RECORD_LIST;
				break;
			}

			if (!tg_print_tags(ctx))
				break;

			char *line = read_input(
			    "Enter tag IDs separated by spaces: ");
			if (!line || line[0] == '\0') {
				free(line);
				break;
			}
			strncpy(tag_name, line, sizeof(tag_name) - 1);
			tag_name[sizeof(tag_name) - 1] = '\0';
			free(line);
				break;

			uint16_t *tag_ids = NULL;
			uint16_t n_tag_ids = 0;
			size_t tag_cap = 5;
			tag_ids = malloc(tag_cap * sizeof(uint16_t));
			if (!tag_ids) {
				ctx->error_status = ERR_INTERNAL;
				break;
			}

			bool parse_ok = true;
			char *tok = strtok(tag_name, " ");
			while (tok) {
				char *end;
				long val = strtol(tok, &end, 10);
				if (end == tok || val < 0 ||
				    val > (long)UINT16_MAX) {
					parse_ok = false;
					break;
				}
				uint16_t tid = (uint16_t)val;
				if (tg_get_tag_name_by_id(ctx, tid) == NULL) {
					ctx->error_status =
					    ERR_TAG_ID_NOT_FOUND;
					parse_ok = false;
					break;
				}
				if (n_tag_ids >= tag_cap) {
					tag_cap *= 2;
					uint16_t *tmp =
					    realloc(tag_ids,
						    tag_cap * sizeof(uint16_t));
					if (!tmp) {
						ctx->error_status =
						    ERR_INTERNAL;
						parse_ok = false;
						break;
					}
					tag_ids = tmp;
				}
				tag_ids[n_tag_ids++] = tid;
				tok = strtok(NULL, " ");
			}

			if (!parse_ok) {
				free(tag_ids);
				if (ctx->error_status == ERR_NONE)
					ctx->error_status = ERR_TAG_NOT_FOUND;
				break;
			}

			if (n_tag_ids == 0) {
				free(tag_ids);
				break;
			}

			uint16_t rec_count;
			uint16_t *rec_ids =
			    tm_query(ctx, tag_ids, n_tag_ids, &rec_count);
			free(tag_ids);
			if (!rec_ids || rec_count == 0) {
				free(rec_ids);
				ctx->error_status = ERR_RECORD_SEARCH_NO_MATCH;
				break;
			}
			char pattern[MAX_NAME_LEN];
			read_regex(ctx, pattern, sizeof(pattern));
			uint16_t match_count;
			uint16_t *match_ids = rd_search_records_in_set(
			    ctx, pattern, rec_ids, rec_count, &match_count);
			free(rec_ids);
			if (!match_ids || match_count == 0) {
				free(match_ids);
				ctx->error_status = ERR_RECORD_SEARCH_NO_MATCH;
				break;
			}
			display_record_list(ctx, match_ids, match_count);
			{
				char *line =
			    read_input("Enter record ID from the list: ");
				if (!line || line[0] == '\0') {
					free(line);
					free(match_ids);
					break;
				}
				char *end;
				long val = strtol(line, &end, 10);
				free(line);
				if (*end != '\0' || val < 0 ||
				    val > (long)UINT16_MAX) {
					free(match_ids);
					break;
				}
				uint16_t chosen = (uint16_t)val;
				bool found = false;
				for (uint16_t i = 0; i < match_count; i++) {
					if (match_ids[i] == chosen) {
						found = true;
						break;
					}
				}
				if (!found) {
					free(match_ids);
					break;
				}
				display_record_with_tags(ctx, chosen);
				free(match_ids);
			}
			break;
		}

		default:
			break;
		}
		printf("\033[H\033[2J\033[3J");
		fflush(stdout);
		if (ctx->error_status != ERR_NONE)
			dv_print_error(ctx, TYPE_APP);
	}
}

static const struct record *_select_record_by_search(dv_ctx_t *ctx)
{
	char pattern[MAX_NAME_LEN];
	read_regex(ctx, pattern, sizeof(pattern));

	uint16_t count;
	uint16_t *ids = rd_search_records_by_name(ctx, pattern, &count);
	if (!ids || count == 0) {
		free(ids);
		ctx->error_status = ERR_RECORD_SEARCH_NO_MATCH;
		return NULL;
	}

	if (count == 1) {
		uint16_t id = ids[0];
		free(ids);
		return rd_get_record_by_id(ctx, id);
	}

	display_record_list(ctx, ids, count);

	for (;;) {
		char *line = read_input("Enter record ID from the list: ");
		if (!line)
			break;
		if (line[0] == '\0') {
			free(line);
			break;
		}

		char *end;
		long val = strtol(line, &end, 10);
		free(line);
		if (*end != '\0' || val < 0 || val > (long)UINT16_MAX) {
			fprintf(stderr, "[!] invalid input.\n");
			continue;
		}

		uint16_t chosen = (uint16_t)val;
		for (uint16_t i = 0; i < count; i++) {
			if (ids[i] == chosen) {
				free(ids);
				return rd_get_record_by_id(ctx, chosen);
			}
		}
		fprintf(stderr, "[!] ID %u is not in the list.\n", chosen);
	}

	free(ids);
	return NULL;
}
