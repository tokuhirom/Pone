#include "pone.h"

// create new pair
pone_val* pone_pair_new(pone_universe* universe, pone_val* key, pone_val* value) {
    pone_val* pair = pone_obj_new(universe, universe->class_pair);
    pone_obj_set_ivar(universe, pair, "$!key", key);
    pone_obj_set_ivar(universe, pair, "$!value", value);
    return pair;
}

PONE_DECLARE_GETTER(meth_pair_key, "$!key")
PONE_DECLARE_GETTER(meth_pair_value, "$!value")

static pone_val* meth_pair_str(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n==0);

    pone_val* buf = pone_str_copy(world->universe, pone_stringify(world, pone_obj_get_ivar(world->universe, self, "$!key")));
    pone_str_append_c(world, buf, "\t", 1);
    pone_str_append(world, buf, pone_obj_get_ivar(world->universe, self, "$!value"));
    return buf;
}

void pone_pair_init(pone_universe* universe) {
    assert(universe->class_pair == NULL);

    universe->class_pair = pone_class_new(universe, "Pair", strlen("Pair"));
    pone_class_push_parent(universe, universe->class_pair, universe->class_any);
    pone_add_method_c(universe, universe->class_pair, "key", strlen("key"), meth_pair_key);
    pone_add_method_c(universe, universe->class_pair, "value", strlen("value"), meth_pair_value);
    pone_add_method_c(universe, universe->class_pair, "Str", strlen("Str"), meth_pair_str);
    pone_class_compose(universe, universe->class_pair);
    pone_universe_set_global(universe, "Pair", universe->class_pair);
}

