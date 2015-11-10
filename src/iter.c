#include "pone.h"

pone_val* pone_iter_init(pone_world* world, pone_val* obj) {
    pone_val* method = pone_find_method(world, obj, "iterator");
    if (pone_defined(method)) {
        // call it.
        return pone_code_call(world, method, obj, 0);
    } else {
        pone_throw_str(world, "There is no 'iterator' method on the object");
        abort();
    }
}

// TODO deprecate this API
pone_val* pone_iter_next(pone_world* world, pone_val* iter) {
    return pone_call_method(world, iter, "pull-one", 0);
}

