#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
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

	char *buffer;
	if (!(buffer = malloc(MAX_PATH_LEN + 1))) {
		rd_destroy_record_list(ctx);
		tm_free(ctx);
		tg_destroy_tag_table(ctx);
		free(ctx);
		return NULL;
	}

	if (!getcwd(buffer, MAX_PATH_LEN)) {
		rd_destroy_record_list(ctx);
		tm_free(ctx);
		tg_destroy_tag_table(ctx);
		free(ctx);
		free(buffer);
		return NULL;
	}

	ctx->save_path = buffer;

	ctx->error_status = ERR_NONE;
	ctx->success_msg = NULL;
	ctx->error_msg = NULL;
	return ctx;
}

void destroy_dv_ctx(dv_ctx_t *ctx)
{
	if (!ctx)
		return;
	tg_destroy_tag_table(ctx);
	rd_destroy_record_list(ctx);
	tm_free(ctx);
	free(ctx->save_path);
	free(ctx);
}

int dv_save(dv_ctx_t *ctx)
{
	char full[MAX_PATH_LEN + sizeof(DV_SAVE_FILE) + 2];
	char tmp[MAX_PATH_LEN + sizeof(DV_SAVE_FILE) +
		 sizeof(DV_SAVE_FILE_TMP_EXT) + 2];

	int n =
	    snprintf(full, sizeof(full), "%s/%s", ctx->save_path, DV_SAVE_FILE);
	if (n < 0 || (size_t)n >= sizeof(full)) {
		ctx->error_status = ERR_INTERNAL;
		ctx->error_msg = "failed to build save path";
		return -1;
	}

	n = snprintf(tmp, sizeof(tmp), "%s%s", full, DV_SAVE_FILE_TMP_EXT);
	if (n < 0 || (size_t)n >= sizeof(tmp)) {
		ctx->error_status = ERR_INTERNAL;
		ctx->error_msg = "failed to build temp save path";
		return -1;
	}

	FILE *f = fopen(tmp, "wb");
	if (!f) {
		ctx->error_status = ERR_INTERNAL;
		ctx->error_msg = strerror(errno);
		return -1;
	}

	uint32_t magic = DV_MAGIC;
	uint8_t ver = DV_VERSION;
	fwrite(&magic, sizeof(magic), 1, f);
	fwrite(&ver, sizeof(ver), 1, f);

	rd_write(ctx, f);
	tg_write(ctx, f);
	tm_write(ctx, f);

	if (fflush(f) == EOF || ferror(f)) {
		fclose(f);
		remove(tmp);
		ctx->error_status = ERR_INTERNAL;
		ctx->error_msg = strerror(errno);
		return -1;
	}
	if (fclose(f) != 0) {
		remove(tmp);
		ctx->error_status = ERR_INTERNAL;
		ctx->error_msg = strerror(errno);
		return -1;
	}

	if (rename(tmp, full) != 0) {
		remove(tmp);
		ctx->error_status = ERR_INTERNAL;
		ctx->error_msg = strerror(errno);
		return -1;
	}

	return 0;
}

int dv_load(dv_ctx_t *ctx, const char *path)
{
	FILE *f = fopen(path, "rb");
	if (!f)
		return -1;

	uint32_t magic;
	uint8_t ver;
	if (fread(&magic, sizeof(magic), 1, f) != 1 ||
	    fread(&ver, sizeof(ver), 1, f) != 1) {
		fclose(f);
		return -1;
	}

	if (magic != DV_MAGIC || ver != DV_VERSION) {
		fclose(f);
		return -1;
	}

	if (rd_read(ctx, f) != 0 || tg_read(ctx, f) != 0 ||
	    tm_read(ctx, f) != 0) {
		fclose(f);
		return -1;
	}

	fclose(f);
	return 0;
}

const char *dv_get_save_path(dv_ctx_t *ctx)
{
	return ctx->save_path;
}

void dv_set_success_msg(dv_ctx_t *ctx, const char *msg)
{
	ctx->success_msg = msg;
}

void dv_set_error_status(dv_ctx_t *ctx, dv_error err)
{
	ctx->error_status = err;
}

void dv_set_error_msg(dv_ctx_t *ctx, const char *msg)
{
	ctx->error_msg = msg;
}

dv_error dv_get_error_status(dv_ctx_t *ctx)
{
	return ctx->error_status;
}

const char *dv_get_success_msg(dv_ctx_t *ctx)
{
	return ctx->success_msg;
}

const struct tag *dv_get_tag_list(dv_ctx_t *ctx)
{
	return ctx->tag_list->data;
}

void dv_print_error(dv_ctx_t *ctx)
{
	display_separator(stderr);

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
	case ERR_INVALID_REGEX:
		fprintf(stderr, "malformed expression\n");
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
	case ERR_USED_RECORD_NAME:
		fprintf(stderr,
			"failed to create record, name already in use\n");
		break;
	case ERR_FILE_EXISTS:
		fprintf(stderr, "a file with that name already exists.\n");
		break;
	case ERR_INVALID_ID_LIST:
		fprintf(stderr, "invalid id list\n");
		break;
	case ERR_INTERNAL:
		fprintf(stderr, "internal error.\n");
		break;
	default:
		fprintf(stderr, "unknown error.\n");
		break;
	}

	if (ctx->error_msg) {
		fprintf(stderr, "%s\n", ctx->error_msg);
	}

	display_separator(stderr);
}

void dv_print_success(dv_ctx_t *ctx)
{
	display_separator(stdout);
	printf("%s\n", ctx->success_msg);
	display_separator(stdout);
}
