#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <regex.h>
#include <curl/curl.h>
#include <readline/readline.h>
#include "validate.h"
#include "devault.h"
#include "devaultInt.h"

static const char *RESERVED_NAMES[] = {
    "CON", "PRN", "AUX", "NUL", ".", "..", NULL};

static int _strcasecmp(const char *a, const char *b)
{
	while (*a && *b) {
		int ca = tolower((unsigned char)*a);
		int cb = tolower((unsigned char)*b);
		if (ca != cb)
			return ca - cb;
		a++;
		b++;
	}
	return tolower((unsigned char)*a) - tolower((unsigned char)*b);
}

static bool _match_regex(const char *pattern, const char *input)
{
	regex_t re;
	int result;

	if (regcomp(&re, pattern, REG_EXTENDED | REG_NOSUB) != 0) {
		return false;
	}
	result = regexec(&re, input, 0, NULL, 0);
	regfree(&re);
	return result == 0;
}

static bool _is_valid_url(dv_ctx_t *ctx, const char *input)
{
	size_t len = strlen(input);

	if (len == 0) {
		ctx->error_status = ERR_EMPTY_INPUT;
		return false;
	}

	if (len > MAX_URL_LEN) {
		ctx->error_status = ERR_EXCEEDS_MAX_LEN;
		return false;
	}

	CURLU *h = curl_url();
	CURLUcode rc = curl_url_set(h, CURLUPART_URL, input, 0);

	if (rc == CURLUE_OK) {
		char *scheme = NULL;

		CURLUcode rc2 = curl_url_get(h, CURLUPART_SCHEME, &scheme, 0);
		if (rc2 != CURLUE_OK || scheme == NULL) {
			curl_url_cleanup(h);
			ctx->error_status = ERR_MALFORMED_URL;
			return false;
		}

		const char *allowed[] = {"https", "http", NULL};
		for (int i = 0; allowed[i] != NULL; i++) {
			if (_strcasecmp(scheme, allowed[i]) == 0) {
				curl_free(scheme);
				curl_url_cleanup(h);
				ctx->error_status = ERR_NONE;
				return true;
			}
		}
		ctx->error_status = ERR_INVALID_SCHEME;
		curl_free(scheme);
		curl_url_cleanup(h);
		return false;
	}

	curl_url_cleanup(h);
	ctx->error_status = ERR_MALFORMED_URL;
	return false;
}

static bool _is_valid_path(dv_ctx_t *ctx, const char *input)
{
	size_t len = strlen(input);

	if (!len) {
		ctx->error_status = ERR_EMPTY_INPUT;
		return false;
	}

	if (len > MAX_PATH_LEN) {
		ctx->error_status = ERR_EXCEEDS_MAX_LEN;
		return false;
	}

	if (_match_regex(REGEX_PATH_TRAVERSAL, input)) {
		ctx->error_status = ERR_PATH_TRAVERSAL;
		return false;
	}

	if (!_match_regex(REGEX_PATH_SAFE, input)) {
		ctx->error_status = ERR_INVALID_CHARS;
		return false;
	}

	ctx->error_status = ERR_NONE;
	return true;
}

static bool _is_valid_filename(dv_ctx_t *ctx, const char *input)
{
	size_t len = strlen(input);

	if (len == 0) {
		ctx->error_status = ERR_EMPTY_INPUT;
		return false;
	}

	if (len > MAX_NAME_LEN) {
		ctx->error_status = ERR_EXCEEDS_MAX_LEN;
		return false;
	}

	for (int i = 0; RESERVED_NAMES[i] != NULL; i++) {
		if (_strcasecmp(input, RESERVED_NAMES[i]) == 0) {
			ctx->error_status = ERR_RESERVED_FILENAME;
			return false;
		}
	}

	if (!_match_regex(REGEX_FILENAME, input)) {
		ctx->error_status = ERR_INVALID_CHARS;
		return false;
	}

	ctx->error_status = ERR_NONE;
	return true;
}

static bool _is_valid_menu_opt(
    dv_ctx_t *ctx, const char *input, int min, int max, int *result)
{
	long parsed;
	size_t len = strlen(input);

	if (len == 0) {
		ctx->error_status = ERR_EMPTY_INPUT;
		return false;
	}

	if (!_match_regex(REGEX_MENU_OPTION, input)) {
		ctx->error_status = ERR_OPTION_NOT_NUMERIC;
		return false;
	}

	parsed = strtol(input, NULL, 10);

	if (parsed < (long)min || parsed > (long)max) {
		ctx->error_status = ERR_OPTION_OUT_OF_RANGE;
		return false;
	}

	*result = (int)parsed;
	ctx->error_status = ERR_NONE;
	return true;
}

static bool _is_valid_alphanum_name(dv_ctx_t *ctx, const char *input)
{
	size_t len = strlen(input);

	if (len == 0) {
		ctx->error_status = ERR_EMPTY_INPUT;
		return false;
	}

	if (len > MAX_NAME_LEN) {
		ctx->error_status = ERR_EXCEEDS_MAX_LEN;
		return false;
	}

	if (isdigit((unsigned char)input[0])) {
		ctx->error_status = ERR_NAME_STARTS_WITH_DIGIT;
		return false;
	}

	if (!_match_regex(REGEX_ALPHANUM_NAME, input)) {
		ctx->error_status = ERR_INVALID_CHARS;
		return false;
	}

	ctx->error_status = ERR_NONE;
	return true;
}

static bool _is_valid_bool_opt(dv_ctx_t *ctx, const char *input)
{
	size_t len = strlen(input);

	if (len == 0) {
		ctx->error_status = ERR_EMPTY_INPUT;
		return false;
	}

	if (len != 1) {
		ctx->error_status = ERR_EXCEEDS_MAX_LEN;
		return false;
	}

	if (input[0] == 'y' || input[0] == 'Y') {
		ctx->error_status = ERR_NONE;
		return true;
	}

	if (input[0] == 'n' || input[0] == 'N') {
		ctx->error_status = ERR_NONE;
		return false;
	}

	ctx->error_status = ERR_OPTION_OUT_OF_RANGE;
	return false;
}

typedef bool (*_validator_fn)(dv_ctx_t *, const char *);

typedef struct {
	const char *prompt;
	InputType error_type;
	_validator_fn validate;
} _input_field;

static const _input_field _INPUT_FIELDS[] = {
    [TYPE_URL] = {"Enter URL: ", TYPE_URL, _is_valid_url},
    [TYPE_PATH] = {"Enter path: ", TYPE_PATH, _is_valid_path},
    [TYPE_NAME] = {"Enter file name: ", TYPE_NAME, _is_valid_filename},
    [TYPE_ALPHANAME] = {"Enter name: ",
			TYPE_ALPHANAME,
			_is_valid_alphanum_name},
};

void read_input(dv_ctx_t *ctx, char *out, size_t max_len, InputType type)
{
	const _input_field *f = &_INPUT_FIELDS[type];
	for (;;) {
		char *line = readline(f->prompt);
		if (line) {
			snprintf(out, max_len, "%s", line);
			free(line);
		} else {
			out[0] = '\0';
		}
		if (f->validate(ctx, out))
			return;
		dv_print_error(ctx);
	}
}

int read_menu_opt(dv_ctx_t *ctx, int min, int max)
{
	char prompt[64];
	snprintf(prompt, sizeof(prompt), "Select option (%d-%d): ", min, max);

	bool valid;
	int result = min;

	do {
		char *line = readline(prompt);
		if (!line) {
			ctx->error_status = ERR_EMPTY_INPUT;
			dv_print_error(ctx);
			continue;
		}
		valid = _is_valid_menu_opt(ctx, line, min, max, &result);
		if (!valid)
			dv_print_error(ctx);
		free(line);
	} while (!valid);

	return result;
}

bool read_bool_opt(dv_ctx_t *ctx, const char *prompt)
{
	for (;;) {
		char full[256];
		snprintf(full, sizeof(full), "%s (y/N): ", prompt);
		char *line = readline(full);
		if (!line) {
			ctx->error_status = ERR_EMPTY_INPUT;
			dv_print_error(ctx);
			continue;
		}
		bool result = _is_valid_bool_opt(ctx, line);
		free(line);
		if (ctx->error_status == ERR_NONE)
			return result;
		dv_print_error(ctx);
	}
}

void read_pattern(dv_ctx_t *ctx, char *out, size_t max_len)
{
	for (;;) {
		char *line = readline("Enter regex pattern: ");
		if (!line) {
			ctx->error_status = ERR_EMPTY_INPUT;
			dv_print_error(ctx);
			continue;
		}

		regex_t re;
		int rc = regcomp(&re, line, REG_EXTENDED | REG_NOSUB);
		if (rc == 0) {
			regfree(&re);
			strncpy(out, line, max_len - 1);
			out[max_len - 1] = '\0';
			free(line);
			return;
		}
		char errbuf[256];
		regerror(rc, &re, errbuf, sizeof(errbuf));
		regfree(&re);

		ctx->error_msg = errbuf;
		dv_print_error(ctx);
		ctx->error_msg = NULL;
		free(line);
	}
}

static bool _is_valid_id_list(dv_ctx_t *ctx, const char *input)
{
	size_t len = strlen(input);

	if (len == 0) {
		ctx->error_status = ERR_NONE;
		return true;
	}

	if (!_match_regex(REGEX_ID_LIST, input)) {
		ctx->error_status = ERR_INVALID_ID_LIST;
		return false;
	}

	ctx->error_status = ERR_NONE;
	return true;
}

uint16_t *read_id_list(dv_ctx_t *ctx, size_t *count, const char *prompt)
{
	for (;;) {
		char *line = readline(prompt);
		if (!line) {
			ctx->error_status = ERR_EMPTY_INPUT;
			dv_print_error(ctx);
			continue;
		}

		if (!_is_valid_id_list(ctx, line)) {
			free(line);
			dv_print_error(ctx);
			continue;
		}

		size_t n = 0;
		{
			const char *p = line;
			char *endptr;
			while (*p != '\0') {
				strtoul(p, &endptr, 10);
				if (endptr == p)
					break;
				n++;
				p = endptr;
			}
		}

		uint16_t *out = malloc((n + 1) * sizeof(uint16_t));
		if (!out) {
			free(line);
			ctx->error_status = ERR_INTERNAL;
			ctx->error_msg = strerror(errno);
			dv_print_error(ctx);
			*count = 0;
			return NULL;
		}

		{
			const char *p = line;
			char *endptr;
			size_t i = 0;
			while (*p != '\0') {
				unsigned long val = strtoul(p, &endptr, 10);
				if (endptr == p)
					break;
				out[i++] = (uint16_t)val;
				p = endptr;
			}
			out[i] = UINT16_MAX;
			*count = n;
		}

		free(line);
		return out;
	}
}
