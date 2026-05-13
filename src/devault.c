#include <stdio.h>
#include "devaultInt.h"
#include "devault.h"

void dv_print_error(dv_ctx_t *ctx, input_type type)
{
	const char *t;

	switch (type) {
	case TYPE_URL:
		t = "URL";
		break;
	case TYPE_PATH:
		t = "path";
		break;
	case TYPE_NAME:
		t = "file name";
		break;
	case TYPE_OPTION:
		t = "menu option";
		break;
	case TYPE_ALPHANAME:
		t = "name";
		break;
	default:
		t = "input";
		break;
	}

	for (int i = 0; i < SEPARATOR_LEN; i++)
		fputc('-', stderr);
	fputc('\n', stderr);

	fprintf(stderr, "[!] Invalid %s — ", t);

	switch (ctx->error_status) {
	case ERR_EMPTY_INPUT:
		fprintf(stderr, "input cannot be empty.\n");
		break;
	case ERR_EXCEEDS_MAX_LEN:
		fprintf(stderr, "input exceeds the maximum allowed length.\n");
		break;
	case ERR_INVALID_CHARS:
		fprintf(stderr,
			"contains illegal characters:\n shell metacharacters "
			"like | ; & < > ` $ are not allowed.\n");
		break;
	case ERR_INVALID_SCHEME:
		fprintf(
		    stderr,
		    "invalid url scheme:\n valid schemes are http and https\n");
		break;
	case ERR_PATH_TRAVERSAL:
		fprintf(stderr,
			"path traversal sequences (../) are not allowed.\n");
		break;
	case ERR_OPTION_NOT_NUMERIC:
		fprintf(stderr, "must be a numeric digit.\n");
		break;
	case ERR_OPTION_OUT_OF_RANGE:
		fprintf(stderr, "value is outside the allowed range.\n");
		break;
	case ERR_NAME_STARTS_WITH_DIGIT:
		fprintf(stderr, "name must start with a letter (a-z, A-Z).\n");
		break;
	case ERR_RESERVED_FILENAME:
		fprintf(stderr,
			"this is a reserved system name and cannot be used.\n");
		break;
	case ERR_MALFORMED_URL:
		fprintf(stderr, "malformed url\n");
		break;
	default:
		fprintf(stderr, "unknown validation error.\n");
		break;
	}

	for (int i = 0; i < SEPARATOR_LEN; i++)
		fputc('-', stderr);
	fputc('\n', stderr);
}
