#include "hashfuncs.h"

uint64_t fnv1a64(const char *buf, size_t len)
{
	uint64_t out = FNV1A64_BASIS;
	size_t i;

	for (i = 0; i < len; i++)
		out = (out ^ buf[i]) * FNV1A64_PRIME;

	return out;
}
