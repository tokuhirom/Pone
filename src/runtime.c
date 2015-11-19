#include "pone.h"

#define PONE_VERSION "pre-alpha"

PONE_FUNC(meth_version) {
    return pone_str_new_const(world, PONE_VERSION, strlen(PONE_VERSION));
}

void pone_runtime_init(pone_world* world) {
    pone_val* klass = pone_class_new(world, "Runtime", strlen("Runtime"));
    pone_add_method_c(world, klass, "version", strlen("version"), meth_version);
    pone_universe_set_global(world->universe, "Runtime", klass);
}

