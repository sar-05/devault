#ifndef RECORDS_H
#define RECORDS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "devault.h"

struct record {
	char name[MAX_NAME_LEN];
	char link[MAX_URL_LEN];
	uint64_t hash;
	uint16_t id;
};

typedef struct _record_list RecordList;

bool rd_create_record_list(dv_ctx_t *ctx, uint16_t capacity);

void rd_destroy_record_list(dv_ctx_t *ctx);

bool rd_create_record(dv_ctx_t *ctx, const char *name, const char *link);

bool rd_delete_record(dv_ctx_t *ctx, const char *name);

void rd_print_records(dv_ctx_t ctx);

int rd_save(dv_ctx_t *ctx, const char *path);

int rd_load(dv_ctx_t *ctx, const char *path);

const struct record *rd_get_record_by_id(dv_ctx_t *ctx, uint16_t id);

uint16_t rd_get_record_id_by_name(dv_ctx_t *ctx, const char *name);

#endif
