#ifndef VALIDATE_H
#define VALIDATE_H

#include <regex.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "devault.h"

#define REGEX_PATH_TRAVERSAL "\\.\\./|/\\.\\."
#define REGEX_PATH_SAFE "^[^|;&<>`'\"$![:cntrl:]]+$"
#define REGEX_FILENAME "^[a-zA-Z0-9][a-zA-Z0-9_.+-]*$"
#define REGEX_MENU_OPTION "^[0-9]+$"
#define REGEX_ALPHANUM_NAME "^[a-zA-Z][a-zA-Z0-9 _-]*$"
#define REGEX_ID_LIST "^[0-9]+(\\s+[0-9]+)*$"

typedef enum { TYPE_URL, TYPE_PATH, TYPE_NAME, TYPE_ALPHANAME } InputType;

bool read_bool_opt(dv_ctx_t *ctx, const char *prompt);

void read_input(dv_ctx_t *ctx, char *out, size_t max_len, InputType type);

int read_menu_opt(dv_ctx_t *ctx, int min, int max);

void read_pattern(dv_ctx_t *ctx, char *out, size_t max_len);

uint16_t *read_id_list(dv_ctx_t *ctx, size_t *count, const char *prompt);

#endif
