#include "pone.h" /* PONE_INC */

/**
 * C level API to create new Code object
 */
pone_val* pone_code_new_c(pone_universe* universe, pone_funcptr_t func) {
    pone_code* cv = (pone_code*)pone_obj_alloc(universe, PONE_CODE);
    cv->func = func;
    cv->lex = NULL;

    return (pone_val*)cv;
}

/**
 * pone level API to create new Code object
 */
pone_val* pone_code_new(pone_world* world, pone_funcptr_t func) {
    world->lex->refcnt++;

    pone_code* cv = (pone_code*)pone_obj_alloc(world->universe, PONE_CODE);
    cv->func = func;
    cv->lex = world->lex;

    // pone_lex_refcnt_inc(world, world->lex);

    return (pone_val*)cv;
}

void pone_code_free(pone_universe* universe, pone_val* v) {
#ifdef TRACE_CODE
    printf("pone_code_free: universe:%x code:%x\n", universe, v);
#endif
    assert(pone_type(v) == PONE_CODE);
    pone_code* cv = (pone_code*)v;
    // cv->lex may NULL if the value was created by pone_code_new_c.

    // pone_lex_refcnt_dec(world, cv->lex);
}

pone_val* pone_code_vcall(pone_world* world, pone_val* code, pone_val* self, int n, va_list args) {
    assert(pone_type(code) == PONE_CODE);

    pone_code* cv = (pone_code*)code;
    if (cv->lex) { //pone level code
        pone_world* new_world = pone_world_new_from_world(world, cv->lex);

        pone_funcptr_t func = cv->func;
        pone_val* retval = func(new_world, self, n, args);
        pone_refcnt_inc(world->universe, retval);
        pone_mortalize(world, retval); // refcnt-- it in upper scope

        pone_world_refcnt_dec(new_world);
        return retval;
    } else {
        pone_funcptr_t func = cv->func;
        return func(world, self, n, args);
    }
}

pone_val* pone_code_call(pone_world* world, pone_val* code, pone_val* self, int n, ...) {
    assert(pone_type(code) == PONE_CODE);

    va_list args;
    va_start(args, n);
    pone_val* retval = pone_code_vcall(world, code, self, n, args);
    va_end(args);

    PONE_YIELD(world->universe);

    return retval;
}

void pone_code_init(pone_universe* universe) {
    assert(universe->class_code == NULL);

    universe->class_code = pone_class_new(universe, "Code", strlen("Code"));
    pone_class_compose(universe, universe->class_code);
}

