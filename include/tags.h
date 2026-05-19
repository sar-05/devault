#ifndef TAGS_H
#define TAGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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

int tg_save(dv_ctx_t *ctx, const char *path);

int tg_load(dv_ctx_t *ctx, const char *path);

uint16_t tg_get_tag_id_by_name(dv_ctx_t *ctx, const char *name);

const char *tg_get_tag_name_by_id(dv_ctx_t *ctx, uint16_t id);

uint16_t *tg_filter_records_by_id(dv_ctx_t *ctx,
				  const char **names,
				  uint16_t len,
				  uint16_t *record_count);

#endif
