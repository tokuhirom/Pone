#include "pone.h" /* PONE_INC */

pone_val* pone_init_any(pone_universe* universe) {
    universe->class_any = pone_class_new(universe, "Any", strlen("Any"));
    pone_class_push_parent(universe, universe->class_any, universe->class_mu);
    pone_class_compose(universe, universe->class_any);
    return universe->class_any;
}

