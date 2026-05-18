#ifndef SPAWN_H
#define SPAWN_H

#include <stdbool.h>

bool fork_to_editor(const char *path);
bool fork_to_pager(const char *path);
bool pipe_to_pager(void (*writer)(void *ctx), void *ctx);

#endif
