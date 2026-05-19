#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "devaultInt.h"
#include "devault.h"
#include "records.h"
#include "tagmatrix.h"
#include "tags.h"
#include "display.h"

dv_ctx_t *create_dv_ctx(void)
{
	dv_ctx_t *ctx = malloc(sizeof(struct _dv_ctx));
	if (!ctx)
		return NULL;

	ctx->tag_mat = NULL;
	ctx->tag_list = NULL;
	ctx->record_list = NULL;

	if (tm_create(ctx, MAX_RECORDS, MAX_TAGS) != 0) {
		free(ctx);
		return NULL;
	}

	if (!rd_create_record_list(ctx, MAX_RECORDS)) {
		tm_free(ctx);
		free(ctx);
		return NULL;
	}

	if (!tg_create_tag_table(ctx, MAX_TAGS)) {
		rd_destroy_record_list(ctx);
		tm_free(ctx);
		free(ctx);
		return NULL;
	}

	ctx->error_status = ERR_NONE;
	return ctx;
}

void destroy_dv_ctx(dv_ctx_t *ctx)
{
	if (!ctx)
		return;
	tg_destroy_tag_table(ctx);
	rd_destroy_record_list(ctx);
	tm_free(ctx);
	free(ctx);
}

void dv_print_error(dv_ctx_t *ctx, input_type type)
{
	display_separator(stderr);

	if (type != TYPE_APP) {
		const char *t;
		switch (type) {
		case TYPE_URL:
			t = "URL";
			break;
		case TYPE_PATH:
			t = "path";
			break;
		case TYPE_NAME:
			t = "file name";
			break;
		case TYPE_OPTION:
			t = "menu option";
			break;
		case TYPE_ALPHANAME:
			t = "name";
			break;
		default:
			t = "input";
			break;
		}
		fprintf(stderr, "[!] Invalid %s — ", t);
	} else {
		fprintf(stderr, "[!] ");
	}

	switch (ctx->error_status) {
	case ERR_EMPTY_INPUT:
		fprintf(stderr, "input cannot be empty.\n");
		break;
	case ERR_EXCEEDS_MAX_LEN:
		fprintf(stderr, "input exceeds the maximum allowed length.\n");
		break;
	case ERR_INVALID_CHARS:
		fprintf(stderr,
			"contains illegal characters:\n shell metacharacters "
			"like | ; & < > ` $ are not allowed.\n");
		break;
	case ERR_INVALID_SCHEME:
		fprintf(
		    stderr,
		    "invalid url scheme:\n valid schemes are http and https\n");
		break;
	case ERR_PATH_TRAVERSAL:
		fprintf(stderr,
			"path traversal sequences (../) are not allowed.\n");
		break;
	case ERR_OPTION_NOT_NUMERIC:
		fprintf(stderr, "must be a numeric digit.\n");
		break;
	case ERR_OPTION_OUT_OF_RANGE:
		fprintf(stderr, "value is outside the allowed range.\n");
		break;
	case ERR_NAME_STARTS_WITH_DIGIT:
		fprintf(stderr, "name must start with a letter (a-z, A-Z).\n");
		break;
	case ERR_RESERVED_FILENAME:
		fprintf(stderr,
			"this is a reserved system name and cannot be used.\n");
		break;
	case ERR_MALFORMED_URL:
		fprintf(stderr, "malformed url\n");
		break;
	case ERR_RECORD_NOT_FOUND:
		fprintf(stderr, "record not found.\n");
		break;
	case ERR_TAG_NOT_FOUND:
		fprintf(stderr, "tag not found.\n");
		break;
	case ERR_TAG_ID_NOT_FOUND:
		fprintf(stderr, "tag ID not found.\n");
		break;
	case ERR_RECORD_CREATE_FAILED:
		fprintf(stderr, "failed to create record.\n");
		break;
	case ERR_TAG_CREATE_FAILED:
		fprintf(stderr, "failed to create tag.\n");
		break;
	case ERR_EMPTY_TAG_LIST:
		fprintf(stderr, "empty tag list.\n");
		break;
	case ERR_EMPTY_RECORD_LIST:
		fprintf(stderr, "empty record list.\n");
		break;
	case ERR_RECORD_SEARCH_NO_MATCH:
		fprintf(stderr, "no records match the search.\n");
		break;
	case ERR_NO_TAGS_IN_RECORD:
		fprintf(stderr, "record doesn't have any tags.\n");
		break;
	case ERR_INTERNAL:
		fprintf(stderr, "internal error.\n");
		break;
	default:
		fprintf(stderr, "unknown error.\n");
		break;
	}

	display_separator(stderr);
}
