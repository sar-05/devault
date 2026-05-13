#ifndef DEVAULTINT_H
#define DEVAULTINT_H

#include <stddef.h>
#include <stdint.h>
#include "tagmatrix.h"
#include "records.h"
#include "tags.h"
#include "validate.h"

struct _dv_ctx {
	TagMatrix *tg;
	TagList *tl;
	RecordList *rl;
	dv_error error_status;
};

#endif
