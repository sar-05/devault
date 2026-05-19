#ifndef SPAWN_H
#define SPAWN_H

#include "devault.h"
#include <stdbool.h>

bool fork_to_editor(const char *path);
bool fork_to_xdg_open(const char *url);
bool fork_to_pager(const char *path);
bool pipe_to_pager(void (*writer)(dv_ctx_t *ctx), dv_ctx_t *ctx);

#endif
