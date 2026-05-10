#ifndef INIT_H
#define INIT_H

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DATA_FILES_NUM 4

struct bfile {
	FILE *fp;
	size_t count;
};

struct data {
	struct bfile resources;
	struct bfile tags;
	struct bfile postings;
	struct bfile filetags;
};

struct data *init(void);
void dinit(struct data *d);

#endif
