#ifndef TAGMATRIX_H
#define TAGMATRIX_H

#include <stdint.h>
#include <stdio.h>
#include "devault.h"

typedef struct _tagmatrix TagMatrix;

int tm_create(dv_ctx_t *ctx, uint16_t m_capacity, uint16_t n_capacity);

void tm_free(dv_ctx_t *ctx);

int tm_write(dv_ctx_t *ctx, FILE *f);

int tm_read(dv_ctx_t *ctx, FILE *f);

void tm_delete_record(dv_ctx_t *ctx, int R);

int tm_alloc_record(dv_ctx_t *ctx);

void tm_delete_tag(dv_ctx_t *ctx, int T);

int tm_alloc_tag(dv_ctx_t *ctx);

uint16_t *tm_query(dv_ctx_t *ctx,
		   const uint16_t *tag_ids,
		   uint16_t n_tag_ids,
		   uint16_t *out_count);

int tm_has_tag(dv_ctx_t *ctx, uint16_t record_id, uint16_t tag_id);

void tm_set_tag(dv_ctx_t *ctx, uint16_t record_id, uint16_t tag_id);

void tm_clear_tag(dv_ctx_t *ctx, uint16_t record_id, uint16_t tag_id);

void tm_print(dv_ctx_t *ctx);

#endif
