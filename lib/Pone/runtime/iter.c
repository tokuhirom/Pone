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
    case PONE_ARRAY_ITER:
        if (iter->as.ary_iter.idx < iter->as.ary_iter.val->as.ary.len) {
            return iter->as.ary_iter.val->as.ary.a[iter->as.ary_iter.idx++];
        } else {
            pone_die(world, pone_obj_alloc(world->universe, PONE_CONTROL_BREAK));
        }
    default:
        pone_die_str(world, "you iterate this type of value.");
    }
}

