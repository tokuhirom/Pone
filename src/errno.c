#include "pone.h"

pone_val* pone_errno_new(pone_world* world, pone_int_t errsv) {
    pone_val* e = pone_obj_new(world, world->universe->class_errno);
    pone_obj_set_ivar(world, e, "$!errno", pone_int_new(world, errsv));
    return e;
}

pone_val* pone_errno(pone_world* world) {
    return pone_errno_new(world, world->errsv);
}

PONE_FUNC(meth_errno_str) {
    PONE_ARG("Errno#Str", "");
    pone_int_t errsv = pone_intify(world, pone_obj_get_ivar(world, self, "$!errno"));
    const char * e = strerror(errsv);
    return pone_str_new_const(world, e, strlen(e));
}

PONE_FUNC(meth_errno_int) {
    PONE_ARG("Errno#Int", "");
    return pone_obj_get_ivar(world, self, "$!errno");
}

// setup builtin Errno class
void pone_errno_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_errno == NULL);

    pone_val* klass = pone_class_new(world, "Errno", strlen("Errno"));
    PONE_REG_METHOD(klass, "Str", meth_errno_str);
    PONE_REG_METHOD(klass, "Int", meth_errno_int);
    pone_class_compose(world, klass);
    universe->class_errno = klass;
}

