#include "init.h"
#include "tags.h"

int main(void)
{

	struct data *d = init();
	if (!d) {
		perror("Error reading data dir");
	}

	dinit(d);

	return 0;
}
