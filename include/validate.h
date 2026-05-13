#ifndef VALIDATE_H
#define VALIDATE_H

#include <stdbool.h>
#include <stddef.h>
#include "devault.h"

#define REGEX_PATH_TRAVERSAL "\\.\\./|/\\.\\."
#define REGEX_PATH_SAFE "^[^|;&<>`'\"$![:cntrl:]]+$"
#define REGEX_FILENAME "^[a-zA-Z0-9][a-zA-Z0-9_.+-]*$"
#define REGEX_MENU_OPTION "^[0-9]+$"
#define REGEX_ALPHANUM_NAME "^[a-zA-Z][a-zA-Z0-9 _-]*$"

bool is_valid_url(dv_ctx_t *ctx, const char *input);

size_t is_valid_path(dv_ctx_t *ctx, const char *input);

bool is_valid_filename(dv_ctx_t *ctx, const char *input);

bool is_valid_alphanum_name(dv_ctx_t *ctx, const char *input);

bool is_valid_menu_opt(
    dv_ctx_t *ctx, const char *input, int min, int max, int *result);

void read_url(dv_ctx_t *ctx, char *buffer, size_t max_len);

void read_path(dv_ctx_t *ctx, char *buffer, size_t max_len);

void read_filename(dv_ctx_t *ctx, char *buffer, size_t max_len);

void read_alphanum_name(dv_ctx_t *ctx, char *buffer, size_t max_len);

int read_menu_opt(dv_ctx_t *ctx, int min, int max);

#endif
