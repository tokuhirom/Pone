#include "pone.h"
#include "pone_module.h"
#include "pone_opaque.h"
#include "pone_exc.h"

#include <math.h>

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

PONE_FUNC(meth_atan2) {
    pone_val* y;
    pone_val* x;
    PONE_ARG("math.atan2", "oo", &y, &x);
    return pone_num_new(world, atan2(pone_numify(world, y), pone_numify(world, x)));
}

PONE_FUNC(meth_cos) {
    pone_val* x;
    PONE_ARG("math.cos", "o", &x);
    return pone_num_new(world, cos(pone_numify(world, x)));
}

PONE_FUNC(meth_sin) {
    pone_val* x;
    PONE_ARG("math.sin", "o", &x);
    return pone_num_new(world, sin(pone_numify(world, x)));
}

PONE_FUNC(meth_exp) {
    pone_val* x;
    PONE_ARG("math.exp", "o", &x);
    return pone_num_new(world, exp(pone_numify(world, x)));
}

PONE_FUNC(meth_sqrt) {
    pone_val* x;
    PONE_ARG("math.sqrt", "o", &x);
    return pone_num_new(world, sqrt(pone_numify(world, x)));
}

PONE_FUNC(meth_log) {
    pone_val* x;
    PONE_ARG("math.log", "o", &x);
    return pone_num_new(world, log(pone_numify(world, x)));
}

void PONE_DLL_math(pone_world* world, pone_val* module) {
    pone_module_put(world, module, "abs", pone_code_new_c(world, meth_abs));
    pone_module_put(world, module, "atan2", pone_code_new_c(world, meth_atan2));
    pone_module_put(world, module, "cos", pone_code_new_c(world, meth_cos));
    pone_module_put(world, module, "sin", pone_code_new_c(world, meth_sin));
    pone_module_put(world, module, "exp", pone_code_new_c(world, meth_exp));
    pone_module_put(world, module, "log", pone_code_new_c(world, meth_log));
    pone_module_put(world, module, "sqrt", pone_code_new_c(world, meth_sqrt));
    pone_module_put(world, module, "PI", pone_num_new(world, M_PI));
}
