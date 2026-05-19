#include <stdio.h>
#include <stdlib.h>
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

	char save_path[MAX_PATH_LEN + sizeof(DV_SAVE_FILE) + 2];
	if (snprintf(save_path, sizeof(save_path), "%s/%s",
	             dv_get_save_path(ctx), DV_SAVE_FILE) < 0) {
		fprintf(stderr, "internal error.\n");
		destroy_dv_ctx(ctx);
		return EXIT_FAILURE;
	}
	dv_load(ctx, save_path);

	if (argc > 1)
		cli_run(ctx, argc, argv);
	else
		tui_run(ctx);

	if (dv_save(ctx) != 0)
		fprintf(stderr, "[!] failed to save state.\n");
	destroy_dv_ctx(ctx);
	return 0;
}
