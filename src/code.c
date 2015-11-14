#include "pone.h" /* PONE_INC */

void pone_code_mark(pone_val* val) {
    if (val->as.code.lex) {
        pone_gc_mark_value(val->as.code.lex);
    }
}

/**
 * C level API to create new Code object
 */
pone_val* pone_code_new_c(pone_world* world, pone_funcptr_t func) {
    pone_code* cv = (pone_code*)pone_obj_alloc(world, PONE_CODE);
    cv->func = func;
    cv->lex = NULL;

    return (pone_val*)cv;
}

/**
 * pone level API to create new Code object
 */
pone_val* pone_code_new(pone_world* world, pone_funcptr_t func) {
    pone_code* cv = (pone_code*)pone_obj_alloc(world, PONE_CODE);
    cv->func = func;
    cv->lex = world->lex;

    return (pone_val*)cv;
}

void pone_code_free(pone_universe* universe, pone_val* v) {
    // printf("pone_code_free: universe:%p code:%p\n", universe, v);
    assert(pone_type(v) == PONE_CODE);
}

pone_val* pone_code_vcall(pone_world* world, pone_val* code, pone_val* self, int n, va_list args) {
    assert(pone_type(code) == PONE_CODE);

    pone_code* cv = (pone_code*)code;
    if (cv->lex) { //pone level code
        // save original lex.
        pone_val* orig_lex = world->lex;
        pone_save_tmp(world, orig_lex);
        // create new lex from Code's saved lex.
        world->lex = cv->lex;

        // call code.
        pone_funcptr_t func = cv->func;
        pone_val* retval = func(world, self, n, args);

        // restore original lex.
        world->lex = orig_lex;

        return retval;
    } else {
        pone_funcptr_t func = cv->func;
        return func(world, self, n, args);
    }
}

pone_val* pone_code_call(pone_world* world, pone_val* code, pone_val* self, int n, ...) {
    assert(pone_alive(code));
    assert(pone_type(code) == PONE_CODE);

    va_list args;
    va_start(args, n);
    pone_val* retval = pone_code_vcall(world, code, self, n, args);
    va_end(args);

    return retval;
}

void pone_code_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_code == NULL);

    universe->class_code = pone_class_new(world, "Code", strlen("Code"));
    pone_class_compose(world, universe->class_code);
}

