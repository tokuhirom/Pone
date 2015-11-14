#include "pone.h" /* PONE_INC */

pone_val* pone_init_cool(pone_world* world) {
    pone_universe* universe = world->universe;
    universe->class_cool = pone_class_new(world, "Cool", strlen("Cool"));
    pone_class_push_parent(world, universe->class_cool, universe->class_any);
    pone_class_compose(world, universe->class_cool);
    return universe->class_cool;
}
