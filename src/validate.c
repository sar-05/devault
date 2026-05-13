#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <regex.h>
#include <curl/curl.h>
#include "validate.h"
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

static void _trim_newline(char *str)
{
	size_t len = strlen(str);
	while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
		str[--len] = '\0';
	}
}

bool is_valid_url(dv_ctx_t *ctx, const char *input)
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

size_t is_valid_path(dv_ctx_t *ctx, const char *input)
{
	size_t len = strlen(input);

	if (!len) {
		ctx->error_status = ERR_EMPTY_INPUT;
		return 0;
	}

	if (len > MAX_PATH_LEN) {
		ctx->error_status = ERR_EXCEEDS_MAX_LEN;
		return 0;
	}

	if (_match_regex(REGEX_PATH_TRAVERSAL, input)) {
		ctx->error_status = ERR_PATH_TRAVERSAL;
		return 0;
	}

	if (!_match_regex(REGEX_PATH_SAFE, input)) {
		ctx->error_status = ERR_INVALID_CHARS;
		return 0;
	}

	ctx->error_status = ERR_NONE;
	return len;
}

bool is_valid_filename(dv_ctx_t *ctx, const char *input)
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

bool is_valid_menu_opt(
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

bool is_valid_alphanum_name(dv_ctx_t *ctx, const char *input)
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

void read_url(dv_ctx_t *ctx, char *buffer, size_t max_len)
{
	bool valid;

	do {
		printf("Enter URL: ");
		if (fgets(buffer, (int)max_len, stdin) == NULL) {
			buffer[0] = '\0';
		}
		_trim_newline(buffer);
		valid = is_valid_url(ctx, buffer);
		if (!valid) {
			dv_print_error(ctx, TYPE_URL);
		}
	} while (!valid);
}

void read_path(dv_ctx_t *ctx, char *buffer, size_t max_len)
{
	bool valid;

	do {
		printf("Enter path: ");
		if (fgets(buffer, (int)max_len, stdin) == NULL) {
			buffer[0] = '\0';
		}
		_trim_newline(buffer);
		valid = is_valid_path(ctx, buffer);
		if (!valid) {
			dv_print_error(ctx, TYPE_PATH);
		}
	} while (!valid);
}

void read_filename(dv_ctx_t *ctx, char *buffer, size_t max_len)
{
	bool valid;

	do {
		printf("Enter file name: ");
		if (fgets(buffer, (int)max_len, stdin) == NULL) {
			buffer[0] = '\0';
		}
		_trim_newline(buffer);
		valid = is_valid_filename(ctx, buffer);
		if (!valid) {
			dv_print_error(ctx, TYPE_NAME);
		}
	} while (!valid);
}

void read_alphanum_name(dv_ctx_t *ctx, char *buffer, size_t max_len)
{
	bool valid;

	do {
		printf("Enter name: ");
		if (fgets(buffer, (int)max_len, stdin) == NULL) {
			buffer[0] = '\0';
		}
		_trim_newline(buffer);
		valid = is_valid_alphanum_name(ctx, buffer);
		if (!valid) {
			dv_print_error(ctx, TYPE_ALPHANAME);
		}
	} while (!valid);
}

int read_menu_opt(dv_ctx_t *ctx, int min, int max)
{
	char buffer[16];
	bool valid;
	int result = min;

	do {
		printf("Select option (%d-%d): ", min, max);
		if (fgets(buffer, sizeof(buffer), stdin) == NULL) {
			buffer[0] = '\0';
		} else if (strchr(buffer, '\n') == NULL) {
			int c;
			while ((c = getchar()) != '\n' && c != EOF)
				;
		}
		_trim_newline(buffer);
		valid = is_valid_menu_opt(ctx, buffer, min, max, &result);
		if (!valid) {
			dv_print_error(ctx, TYPE_OPTION);
		}
	} while (!valid);

	return result;
}
