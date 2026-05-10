#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "tags.h"
#include "hashfuncs.h"

#define MAX_NAME_LEN 64

static bool _valid_tag_name(char *name)
{
	int i;
	for (i = 0; *name != '\0'; name++, i++) {
		if (!isalnum(*name) || i > MAX_NAME_LEN - 1) {
			return false;
		}
	}
	return true;
}

bool tg_new_tag(struct tag *tg, char *name)
{
	tg = malloc(sizeof(struct tag));

	if (!tg) {
		perror("malloc");
		return false;
	}

	if ((_valid_tag_name(name))) {
		/* Returns the amount of characters that would have been written */
		int i;
		i = snprintf(tg->name, sizeof(tg->name), "%s", name);
		tg->id = fnv1a64(tg->name, i);
	} else {
		return false;
	}

	return true;
}

// void tg_

void tg_new_resource(void);
