#include "pone.h"

#define PONE_VERSION "pre-alpha"

void pone_runtime_init(pone_world* world) {
    pone_val* klass = pone_class_new(world, "Runtime", strlen("Runtime"));
    pone_obj_set_ivar(world, klass, "version", pone_str_new_const(world, PONE_VERSION, strlen(PONE_VERSION)));
    pone_universe_set_global(world->universe, "Runtime", klass);
}

