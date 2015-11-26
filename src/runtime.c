#include "pone.h"

#define PONE_VERSION "pre-alpha"

void pone_runtime_init(pone_world* world) {
    pone_universe_set_global(world->universe, "$PONE_VERSION", pone_str_new_const(world, PONE_VERSION, strlen(PONE_VERSION)));
}
