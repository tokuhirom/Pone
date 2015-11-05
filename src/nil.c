#include "pone.h" /* PONE_INC */

static pone_nil_t pone_nil_val = { PONE_NIL, -1, PONE_FLAGS_GLOBAL };

pone_val* pone_nil() {
    return (pone_val*)&pone_nil_val;
}

static pone_val* meth_nil_gist(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);
    return pone_str_new_const(world->universe, "Nil", 3);
}

static pone_val* meth_nil_str(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);
    pone_warn_str(world, "Use of Nil in string context");
    return pone_str_new_const(world->universe, "", 0);
}

static pone_val* meth_nil_int(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);
    pone_warn_str(world, "Use of Nil in numeric context");
    return pone_int_new(world->universe, 0);
}

static pone_val* meth_nil_num(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);
    pone_warn_str(world, "Use of Nil in numeric context");
    return pone_num_new(world->universe, 0);
}

void pone_nil_init(pone_universe* universe) {
    assert(universe->class_nil == NULL);

    universe->class_nil = pone_class_new(universe, "Nil", strlen("Nil"));
    pone_class_push_parent(universe, universe->class_nil, universe->class_cool);
    pone_add_method_c(universe, universe->class_nil, "gist", strlen("gist"), meth_nil_gist);
    pone_add_method_c(universe, universe->class_nil, "Str", strlen("Str"), meth_nil_str);
    pone_add_method_c(universe, universe->class_nil, "Int", strlen("Int"), meth_nil_int);
    pone_add_method_c(universe, universe->class_nil, "Num", strlen("Num"), meth_nil_num);
    pone_class_compose(universe, universe->class_nil);
}

