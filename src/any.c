#include "pone.h" /* PONE_INC */

pone_val* pone_init_any(pone_world* world) {
    pone_universe* universe = world->universe;
    universe->class_any = pone_class_new(world, "Any", strlen("Any"));
    pone_class_push_parent(world, universe->class_any, universe->class_mu);
    pone_class_compose(world, universe->class_any);
    return universe->class_any;
}

