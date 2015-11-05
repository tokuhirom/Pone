#include "pone.h" /* PONE_INC */
#include <stdbool.h>

pone_bool pone_true_val = { PONE_BOOL, -1, PONE_FLAGS_GLOBAL, true };
pone_bool pone_false_val = { PONE_BOOL, -1, PONE_FLAGS_GLOBAL, false };

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

static pone_val* meth_bool_str(pone_world* world, pone_val* self, int n, va_list args) {
    if (pone_bool_val(self)) {
        return pone_str_new_const(world->universe, "True", strlen("True"));
    } else {
        return pone_str_new_const(world->universe, "False", strlen("False"));
    }
}

void pone_bool_init(pone_universe* universe) {
    assert(universe->class_bool == NULL);

    universe->class_bool = pone_class_new(universe, "Bool", strlen("Bool"));
    pone_class_push_parent(universe, universe->class_bool, universe->class_cool);
    pone_add_method_c(universe, universe->class_bool, "Str", strlen("Str"), meth_bool_str);
}

