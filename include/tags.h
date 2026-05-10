#ifndef TAGS_H
#define TAGS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>

#define MAX_FILENAME_NAME_LEN 64
#define MAX_FILENAME_EXT_LEN 16
#define MAX_TAG_NAME_LEN 64
#define MAX_PATH_LEN 512

struct record {
	char path[512];
	uint64_t id;
	/* offset to the array of tag ids in filetags.dat */
	off_t off;
	bool active;
};

struct tag {
	char name[MAX_TAG_NAME_LEN];
	uint64_t id;
	/* offset to the postings (array of record ids) in postings.dat */
	off_t off;
	size_t count;
	bool active;
};

#endif
