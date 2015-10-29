#include "pone.h" /* PONE_INC */

pone_val* pone_num_new(pone_universe* universe, double n) {
    pone_num* nv = (pone_num*)pone_obj_alloc(universe, PONE_NUM);
    nv->n = n;
    return (pone_val*)nv;
}

inline double pone_num_val(pone_val* val) {
    assert(pone_type(val) == PONE_NUM);
    return ((pone_num*)val)->n;
}

