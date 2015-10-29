#include "pone.h"

pone_val* pone_iter_init(pone_world* world, pone_val* obj) {
    pone_val* method = pone_find_method(world, obj, "iterator");
    if (pone_defined(method)) {
        // call it.
        return pone_code_call(world, method, 1, obj);
    } else {
        pone_die_str(world, "There is no 'iterator' method on the object");
    }
}

// TODO deprecate this API
pone_val* pone_iter_next(pone_world* world, pone_val* iter) {
    return pone_call_method(world, iter, "pull-one", 1, iter);
}

