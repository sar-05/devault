#ifndef TAGMATRIX_H
#define TAGMATRIX_H

#include <stdint.h>
#include "devault.h"

typedef struct _tagmatrix TagMatrix;

int tm_create(dv_ctx_t *ctx, uint16_t m_capacity, uint16_t n_capacity);

void tm_free(dv_ctx_t *ctx);

int tm_save(dv_ctx_t *ctx, const char *path);

int tm_load(dv_ctx_t *ctx, const char *path);

void tm_delete_record(dv_ctx_t *ctx, int R);

int tm_alloc_record(dv_ctx_t *ctx);

void tm_delete_tag(dv_ctx_t *ctx, int T);

int tm_alloc_tag(dv_ctx_t *ctx);

int *tm_query(dv_ctx_t *ctx,
	      const int *tag_ids,
	      int n_tag_ids,
	      int *out_count);

void tm_print(dv_ctx_t *ctx);

#endif
