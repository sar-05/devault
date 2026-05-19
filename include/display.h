#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>
#include <stdint.h>
#include "devault.h"

void display_record_with_tags(dv_ctx_t *ctx, uint16_t record_id);

void display_all_records(dv_ctx_t *ctx);

void display_record_list(dv_ctx_t *ctx, const uint16_t *ids, uint16_t count);

void display_separator(FILE *fp);

#endif
