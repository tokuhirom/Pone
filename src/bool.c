#include "pone.h" /* PONE_INC */
#include <stdbool.h>

pone_bool pone_true_val = { .type = PONE_BOOL, .flags = PONE_FLAGS_GLOBAL, .b = true };
pone_bool pone_false_val = { .type = PONE_BOOL, .flags = PONE_FLAGS_GLOBAL, .b = false };

inline bool pone_bool_val(pone_val* val) {
    assert(pone_type(val) == PONE_BOOL);
    return ((pone_bool*)val)->b;
}

inline pone_val* pone_true() {
    return (pone_val*)&pone_true_val;
}

inline pone_val* pone_false() {
    return (pone_val*)&pone_false_val;
}

PONE_FUNC(meth_bool_int) {
    PONE_ARG("Bool#Int", "");
    if (pone_bool_val(self)) {
        return pone_int_new(world, 1);
    } else {
        return pone_int_new(world, 0);
    }
}

PONE_FUNC(meth_bool_str) {
    PONE_ARG("Bool#Str", "");

    if (pone_bool_val(self)) {
        return pone_str_new_const(world, "True", strlen("True"));
    } else {
        return pone_str_new_const(world, "False", strlen("False"));
    }
}

void pone_bool_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_bool == NULL);

    universe->class_bool = pone_class_new(world, "Bool", strlen("Bool"));
    pone_add_method_c(world, universe->class_bool, "Str", strlen("Str"), meth_bool_str);
    pone_add_method_c(world, universe->class_bool, "Int", strlen("Int"), meth_bool_int);
    pone_class_compose(world, universe->class_bool);
}
