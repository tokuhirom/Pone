#include "pone.h" /* PONE_INC */
#include "pone_exc.h"

static pone_nil_t pone_nil_val = {
    .type = PONE_NIL,
    .flags = PONE_FLAGS_GLOBAL
};

pone_val* pone_nil() {
    return (pone_val*)&pone_nil_val;
}

PONE_FUNC(meth_nil_gist) {
    PONE_ARG("Nil#gist", "");
    return pone_str_new_const(world, "nil", 3);
}

PONE_FUNC(meth_nil_str) {
    PONE_ARG("Nil#Str", "");
    pone_warn_str(world, "Use of Nil in string context");
    return pone_str_new_const(world, "", 0);
}

PONE_FUNC(meth_nil_int) {
    PONE_ARG("Nil#Int", "");
    pone_warn_str(world, "Use of Nil in numeric context");
    return pone_int_new(world, 0);
}

PONE_FUNC(meth_nil_num) {
    PONE_ARG("Nil#Num", "");
    pone_warn_str(world, "Use of Nil in numeric context");
    return pone_num_new(world, 0);
}

void pone_nil_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_nil == NULL);

    universe->class_nil = pone_class_new(world, "Nil", strlen("Nil"));
    pone_add_method_c(world, universe->class_nil, "gist", strlen("gist"), meth_nil_gist);
    pone_add_method_c(world, universe->class_nil, "Str", strlen("Str"), meth_nil_str);
    pone_add_method_c(world, universe->class_nil, "Int", strlen("Int"), meth_nil_int);
    pone_add_method_c(world, universe->class_nil, "Num", strlen("Num"), meth_nil_num);

    pone_universe_set_global(universe, "Nil", universe->class_nil);
}
