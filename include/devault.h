#ifndef DEVAULT_H
#define DEVAULT_H

#include <stddef.h>

#define MAX_NAME_LEN 64
#define MAX_PATH_LEN 512
#define MAX_URL_LEN 512
#define MAX_RECORDS 1000
#define MAX_TAGS 100

#define SEPARATOR_LEN 48

typedef enum {
	ERR_NONE = 0,
	ERR_EMPTY_INPUT,
	ERR_EXCEEDS_MAX_LEN,
	ERR_MALFORMED_URL,
	ERR_INVALID_SCHEME,
	ERR_INVALID_CHARS,
	ERR_PATH_TRAVERSAL,
	ERR_OPTION_NOT_NUMERIC,
	ERR_OPTION_OUT_OF_RANGE,
	ERR_NAME_STARTS_WITH_DIGIT,
	ERR_RESERVED_FILENAME
} dv_error;

typedef enum {
	TYPE_URL,
	TYPE_PATH,
	TYPE_NAME,
	TYPE_OPTION,
	TYPE_ALPHANAME
} input_type;

typedef struct _dv_ctx dv_ctx_t;

dv_ctx_t *create_dv_ctx(void);

void dv_print_error(dv_ctx_t *ctx, input_type type);

#endif
