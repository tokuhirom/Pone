#include "pone.h" /* PONE_INC */
#include "pone_khash.h"

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

void pone_code_bind(pone_world* world, pone_val* code, const char* key, pone_val* val) {
    assert(pone_type(code) == PONE_CODE);
    assert(code->as.code.lex);

    khash_t(str)* lex = code->as.code.lex->as.lex.map;
    int ret;
    char* ks = pone_strdup(world, key, strlen(key));
    khint_t k = kh_put(str, lex, ks, &ret);
    if (ret == -1) {
        abort(); // TODO better error msg
    } else if (ret == 0) {
        pone_free(world, ks);
    }
    kh_val(lex, k) = val;
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

void pone_push_caller(pone_world* world, const char* filename, int lineno, const char* sub) {
    pone_ary_push(world, world->caller_stack,
                  pone_ary_new(world, 3,
                               pone_str_new_const(world, filename, strlen(filename)),
                               pone_int_new(world, lineno),
                               pone_str_new_const(world, sub, strlen(sub))));
}

void pone_pop_caller(pone_world* world) {
    pone_ary_pop(world, world->caller_stack);
}

pone_val* pone_code_vcall(pone_world* world, const char* filename, int lineno, const char* sub, pone_val* code, pone_val* self, int n, va_list args) {
    assert(pone_type(code) == PONE_CODE);

    if (world->gc_requested) {
        pone_gc_run(world);
    }

    pone_push_caller(world, filename, lineno, sub);

    pone_code* cv = (pone_code*)code;
    if (cv->lex) { //pone level code
        // save original lex.
        pone_val* orig_lex = world->lex;
        if (orig_lex) {
            pone_save_tmp(world, orig_lex);
        }
        // create new lex from Code's saved lex.
        world->lex = cv->lex;

        // call code.
        pone_funcptr_t func = cv->func;
        pone_val* retval = func(world, self, n, args);

        // restore original lex.
        world->lex = orig_lex;

        pone_pop_caller(world);

        return retval;
    } else {
        pone_funcptr_t func = cv->func;
        pone_val* retval = func(world, self, n, args);
        pone_pop_caller(world);
        return retval;
    }
}

pone_val* pone_code_call(pone_world* world, const char* filename, int lineno, const char* sub, pone_val* code, pone_val* self, int n, ...) {
    assert(pone_alive(code));
    assert(pone_type(code) == PONE_CODE);
    // TODO support callable object

    va_list args;
    va_start(args, n);
    pone_val* retval = pone_code_vcall(world, filename, lineno, sub, code, self, n, args);
    va_end(args);

    return retval;
}

void pone_code_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_code == NULL);

    universe->class_code = pone_class_new(world, "Code", strlen("Code"));
    pone_universe_set_global(world->universe, "Code", universe->class_code);
}
