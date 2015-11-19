#include "pone.h" /* PONE_INC */

static pone_nil_t pone_nil_val = {
    .type=PONE_NIL,
    .flags=PONE_FLAGS_GLOBAL
};

pone_val* pone_nil() {
    return (pone_val*)&pone_nil_val;
}

PONE_FUNC(meth_nil_gist) {
    PONE_ARG("Nil#gist", "");
    return pone_str_new_const(world, "Nil", 3);
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
    pone_class_push_parent(world, universe->class_nil, universe->class_cool);
    PONE_REG_METHOD(universe->class_nil, "gist", meth_nil_gist);
    PONE_REG_METHOD(universe->class_nil, "Str", meth_nil_str);
    PONE_REG_METHOD(universe->class_nil, "Int", meth_nil_int);
    PONE_REG_METHOD(universe->class_nil, "Num", meth_nil_num);
    pone_class_compose(world, universe->class_nil);

    pone_universe_set_global(universe, "Nil", pone_nil());
}

