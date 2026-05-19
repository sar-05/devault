#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdio.h>
#include <stdint.h>
#include "devault.h"

struct record;
struct tag;

void display_record(dv_ctx_t *ctx, const struct record *r);

void display_record_by_id(dv_ctx_t *ctx, uint16_t id);

void display_records_by_id(dv_ctx_t *ctx, const uint16_t *ids, uint16_t count);

void display_all_records(dv_ctx_t *ctx);

void display_tag(dv_ctx_t *ctx, const struct tag *t);

void display_tag_by_id(dv_ctx_t *ctx, uint16_t id);

void display_tags_by_id(dv_ctx_t *ctx, const uint16_t *ids, uint16_t count);

void display_record_tags(dv_ctx_t *ctx, uint16_t record_id);

void display_all_tags(dv_ctx_t *ctx);

void display_separator(FILE *fp);

void display_pie_chart(dv_ctx_t *ctx);
void display_bar_chart(dv_ctx_t *ctx);

#endif
