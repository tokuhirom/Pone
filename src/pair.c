#include "pone.h"

// create new pair
pone_val* pone_pair_new(pone_world* world, pone_val* key, pone_val* value) {
    pone_universe* universe = world->universe;
    pone_val* pair = pone_obj_new(world, universe->class_pair);
    pone_obj_set_ivar(world, pair, "$!key", key);
    pone_obj_set_ivar(world, pair, "$!value", value);
    return pair;
}

PONE_DECLARE_GETTER(meth_pair_key, "$!key")
PONE_DECLARE_GETTER(meth_pair_value, "$!value")

PONE_FUNC(meth_pair_str) {
    PONE_ARG("Pair#Str", "");

    pone_val* buf = pone_str_copy(world, pone_stringify(world, pone_obj_get_ivar(world, self, "$!key")));
    pone_str_append_c(world, buf, "=>", 1);
    pone_str_append(world, buf, pone_obj_get_ivar(world, self, "$!value"));
    return buf;
}

void pone_pair_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_pair == NULL);

    universe->class_pair = pone_class_new(world, "Pair", strlen("Pair"));
    pone_add_method_c(world, universe->class_pair, "key", strlen("key"), meth_pair_key);
    pone_add_method_c(world, universe->class_pair, "value", strlen("value"), meth_pair_value);
    pone_add_method_c(world, universe->class_pair, "Str", strlen("Str"), meth_pair_str);
    pone_class_compose(world, universe->class_pair);
    pone_universe_set_global(universe, "Pair", universe->class_pair);
}

