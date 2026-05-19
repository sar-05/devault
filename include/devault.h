#ifndef DEVAULT_H
#define DEVAULT_H

#include <stddef.h>

#define MAX_NAME_LEN 64
#define MAX_PATH_LEN 512
#define MAX_URL_LEN 512
#define MAX_RECORDS 1000
#define MAX_REGEX_LEN 50
#define MAX_TAGS 100

#define SEPARATOR_LEN 48

#define DV_SAVE_FILE "devault.dat"
#define DV_SAVE_FILE_TMP_EXT ".tmp"
#define DV_MAGIC 0x44455641u
#define DV_VERSION 1

typedef enum {
	ERR_NONE = 0,
	ERR_EMPTY_INPUT,
	ERR_EXCEEDS_MAX_LEN,
	ERR_MALFORMED_URL,
	ERR_INVALID_SCHEME,
	ERR_INVALID_REGEX,
	ERR_INVALID_CHARS,
	ERR_PATH_TRAVERSAL,
	ERR_OPTION_NOT_NUMERIC,
	ERR_OPTION_OUT_OF_RANGE,
	ERR_NAME_STARTS_WITH_DIGIT,
	ERR_RESERVED_FILENAME,
	ERR_RECORD_NOT_FOUND,
	ERR_TAG_NOT_FOUND,
	ERR_TAG_ID_NOT_FOUND,
	ERR_RECORD_CREATE_FAILED,
	ERR_TAG_CREATE_FAILED,
	ERR_NO_TAGS_IN_RECORD,
	ERR_EMPTY_TAG_LIST,
	ERR_EMPTY_RECORD_LIST,
	ERR_USED_RECORD_NAME,
	ERR_RECORD_SEARCH_NO_MATCH,
	ERR_INVALID_ID_LIST,
	ERR_FILE_EXISTS,
	ERR_INTERNAL
} dv_error;

typedef struct _dv_ctx dv_ctx_t;

dv_ctx_t *create_dv_ctx(void);

void destroy_dv_ctx(dv_ctx_t *ctx);

void dv_print_error(dv_ctx_t *ctx);

int dv_save(dv_ctx_t *ctx);

int dv_load(dv_ctx_t *ctx, const char *path);

const char *dv_get_save_path(dv_ctx_t *ctx);

void dv_set_success_msg(dv_ctx_t *ctx, const char *msg);

void dv_set_error_status(dv_ctx_t *ctx, dv_error err);

void dv_set_error_msg(dv_ctx_t *ctx, const char *msg);

dv_error dv_get_error_status(dv_ctx_t *ctx);

const char *dv_get_success_msg(dv_ctx_t *ctx);

void dv_print_success(dv_ctx_t *ctx);

const struct tag *dv_get_tag_list(dv_ctx_t *ctx);

#endif
