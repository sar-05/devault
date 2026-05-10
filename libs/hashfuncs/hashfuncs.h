#ifndef HASHFUNCS_H
#define HASHFUNCS_H

#include <stddef.h>
#include <stdint.h>

#define FNV1A64_BASIS 0xcbf29ce484222325
#define FNV1A64_PRIME 0x00000100000001b3

uint64_t fnv1a64(const char *buf, size_t len);

#endif
