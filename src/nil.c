#include "pone.h" /* PONE_INC */

static pone_nil_t pone_nil_val = { PONE_NIL, -1, PONE_FLAGS_GLOBAL };

pone_val* pone_nil() {
    return (pone_val*)&pone_nil_val;
}

static pone_val* meth_nil_int(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);
    pone_warn_str(world, "Use of Nil in numeric context");
    return pone_int_new(world->universe, 0);
}

void pone_nil_init(pone_universe* universe) {
    assert(universe->class_nil == NULL);

    universe->class_nil = pone_class_new(universe, "Nil", strlen("Nil"));
    pone_class_push_parent(universe, universe->class_nil, universe->class_cool);
    pone_add_method_c(universe, universe->class_nil, "Int", strlen("Int"), meth_nil_int);
    pone_class_compose(universe, universe->class_nil);
}

