#include "pone.h" /* PONE_INC */

pone_val* pone_init_cool(pone_universe* universe) {
    universe->class_cool = pone_class_new(universe, "Cool", strlen("Cool"));
    pone_class_push_parent(universe, universe->class_cool, universe->class_any);
    return universe->class_cool;
}
