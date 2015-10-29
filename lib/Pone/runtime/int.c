#include "pone.h" /* PONE_INC */

inline int pone_int_val(pone_val* val) {
    assert(pone_type(val) == PONE_INT);
    return ((pone_int*)val)->i;
}

pone_val* pone_int_new(pone_universe* universe, int i) {
    pone_int* iv = (pone_int*)pone_obj_alloc(universe, PONE_INT);
    iv->i = i;
    return (pone_val*)iv;
}

// ++$i
pone_val* pone_int_incr(pone_world* world, pone_val* i) {
    if (pone_is_frozen(i)) {
        // TODO should we use better interface?
        // show line number
        pone_dd(world->universe, i);
        fprintf(stderr, "[ERROR] You can't modify an itenger literal");
        exit(1);
    }
    ((pone_int*)i)->i++;
}

