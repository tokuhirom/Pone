#include "pone.h"

pone_val* pone_iter_init(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_ARRAY:
        return pone_ary_iter_new(world->universe, val);
    default:
        pone_die_str(world, "you iterate this type of value.");
    }
}

pone_val* pone_iter_next(pone_world* world, pone_val* iter) {
    switch (pone_type(iter)) {
    case PONE_OBJ: {
        pone_val* code = pone_find_method(world, iter, "ITER-NEXT");
        return pone_code_call(world, code, 1, iter);
    }
    default:
        pone_die_str(world, "you can't iterate non-iterable objects");
    }
}

