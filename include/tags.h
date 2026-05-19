#ifndef TAGS_H
#define TAGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "devault.h"

struct tag {
	char name[MAX_NAME_LEN];
	uint64_t hash;
	uint16_t id;
};

typedef struct _tag_list TagList;

bool tg_create_tag_table(dv_ctx_t *ctx, uint16_t capacity);

void tg_destroy_tag_table(dv_ctx_t *ctx);

bool tg_create_tag(dv_ctx_t *ctx, const char *name);

bool tg_delete_tag(dv_ctx_t *ctx, const char *name);

bool tg_print_tags(dv_ctx_t *ctx);

int tg_write(dv_ctx_t *ctx, FILE *f);

int tg_read(dv_ctx_t *ctx, FILE *f);

uint16_t tg_get_tag_id_by_name(dv_ctx_t *ctx, const char *name);

const char *tg_get_tag_name_by_id(dv_ctx_t *ctx, uint16_t id);

bool tg_is_tag_list_empty(dv_ctx_t *ctx);

#endif
