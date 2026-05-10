#include "init.h"
#include <stdio.h>
#include <stdlib.h>

static const char *df[] = {"./.data/records.dat",
			   "./.data/tags.dat",
			   "./.data/postings.dat",
			   "./.data/filetags.dat"};

struct data *init(void)
{
	char const *dir = "./.data";
	struct stat sb;

	if (stat(dir, &sb) == -1 || !S_ISDIR(sb.st_mode)) {
		if (mkdir(dir, 0755) == -1) {
			perror("mkdir");
			return NULL;
		}
	}

	struct data *d = malloc(sizeof(struct data));
	if (!d) {
		perror("malloc");
		return NULL;
	}

	struct bfile *files[] = {
	    &d->resources, &d->tags, &d->postings, &d->filetags};

	int i;
	for (i = 0; i < DATA_FILES_NUM; i++) {
		files[i]->fp = fopen(df[i], "ab+");
		if (!files[i]->fp) {
			perror("fopen");
			while (--i >= 0)
				fclose(files[i]->fp);
			free(d);
			return NULL;
		}

		rewind(files[i]->fp);

		size_t count;
		size_t n = fread(&count, sizeof(size_t), 1, files[i]->fp);

		if (n == 1) {
			files[i]->count = count;
		} else if (feof(files[i]->fp)) {
			files[i]->count = 0;
			count = 0;
			if (fwrite(&count, sizeof(size_t), 1, files[i]->fp) !=
			    1) {
				fprintf(stderr,
					"Failed to initialize sentinel: %s\n",
					df[i]);
				fclose(files[i]->fp);
				while (--i >= 0)
					fclose(files[i]->fp);
				free(d);
				return NULL;
			}
		} else {
			fprintf(stderr, "Corrupted file: %s\n", df[i]);
			fclose(files[i]->fp);
			while (--i >= 0)
				fclose(files[i]->fp);
			free(d);
			return NULL;
		}
	}

	return d;
}

void dinit(struct data *d)
{
	fclose(d->resources.fp);
	fclose(d->tags.fp);
	fclose(d->postings.fp);
	fclose(d->filetags.fp);

	free(d);
}
