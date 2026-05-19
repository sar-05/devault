#include <stdio.h>
#include "devault.h"
#include "tui.h"
#include "cli.h"

int main(int argc, char *argv[])
{
	dv_ctx_t *ctx = create_dv_ctx();
	if (!ctx) {
		fprintf(stderr, "Failed to initialize context\n");
		return 1;
	}

	if (argc > 1)
		cli_run(ctx, argc, argv);
	else
		tui_run(ctx);

	destroy_dv_ctx(ctx);
	return 0;
}
