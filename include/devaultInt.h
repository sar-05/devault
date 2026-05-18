#ifndef DEVAULTINT_H
#define DEVAULTINT_H

#include <stddef.h>
#include <stdint.h>
#include "tagmatrix.h"
#include "records.h"
#include "tags.h"
#include "validate.h"

typedef struct _tag_matrix_header {
	uint32_t magic;
	uint16_t m_capacity;
	uint16_t n_capacity;
	uint16_t m_used;
	uint16_t n_used;
	uint16_t fl_head;
	uint8_t version;
	uint8_t fl_tag_count;
} TagMatrixHeader;

struct _tagmatrix {
	TagMatrixHeader hdr;
	uint8_t _padding[16];

	uint8_t fl_tag_ids[MAX_TAGS];
	uint8_t *data;
	int row_stride;
};

typedef struct _tag_list_header {
	uint32_t magic;
	uint16_t capacity;
	uint16_t used;
} TagListHeader;

struct _tag_list {
	TagListHeader hdr;
	struct tag *data;
};

typedef struct _record_list_header {
	uint32_t magic;
	uint16_t capacity;
	uint16_t used;
} RecordListHeader;

struct _record_list {
	RecordListHeader hdr;
	struct record *data;
};

struct _dv_ctx {
	TagMatrix *tag_mat;
	TagList *tag_list;
	RecordList *record_list;
	dv_error error_status;
};

#endif
