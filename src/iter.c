#include "pone.h"
#include "pone_exc.h"

pone_val* pone_iter_init(pone_world* world, pone_val* obj) {
    assert(pone_alive(obj));

    pone_val* method = pone_find_method(world, obj, "iterator");
    if (method && pone_defined(method)) {
        // call it.
        return pone_code_call(world, __FILE__, __LINE__, __func__, method, obj, 0);
    } else {
        pone_throw_str(world, "There is no 'iterator' method on the object");
        abort();
    }
}

// TODO deprecate this API
pone_val* pone_iter_next(pone_world* world, pone_val* iter) {
    assert(pone_alive(iter));
    return pone_call_method(world, __FILE__, __LINE__, __func__, iter, "pull-one", 0);
}
