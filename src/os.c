#include "pone.h"

PONE_FUNC(meth_getpid) {
    PONE_ARG("OS#getpid", "");
    return pone_int_new(world, getpid());
}

void pone_os_init(pone_world* world) {
    pone_val* klass = pone_class_new(world, "OS", strlen("OS"));
    PONE_REG_METHOD(klass, "getpid", meth_getpid);
    pone_universe_set_global(world->universe, "OS", klass);
}

