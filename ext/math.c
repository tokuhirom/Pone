#include "pone.h"
#include "pone_module.h"
#include "pone_opaque.h"
#include "pone_exc.h"

PONE_FUNC(meth_abs) {
    pone_val* val;
    PONE_ARG("abs", "o", &val);
    switch (pone_type(val)) {
    case PONE_INT: {
        pone_int_t i = pone_int_val(val);
        if (i < 0) {
            return pone_int_new(world, -i);
        } else {
            return val;
        }
    }
    case PONE_NUM: {
        pone_num_t i = pone_num_val(val);
        if (i < 0) {
            return pone_num_new(world, -i);
        } else {
            return val;
        }
    }
    default:
        pone_throw_str(world, "you can't call abs() for non-numeric value");
        abort();
    }
}

void PONE_DLL_math(pone_world* world, pone_val* module) {
    pone_module_put(world, module, "abs", pone_code_new_c(world, meth_abs));
}
