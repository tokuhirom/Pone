#include "pone.h" /* PONE_INC */

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

    // pone_lex_refcnt_dec(world, cv->lex);
}

pone_val* pone_code_call(pone_world* world, pone_val* code, int n, ...) {
    assert(pone_type(code) == PONE_CODE);

    pone_code* cv = (pone_code*)code;
    assert(cv->lex != NULL);
    pone_world* new_world = pone_new_world_from_world(world, cv->lex);

    va_list args;
    va_start(args, n);
    pone_funcptr_t func = cv->func;
    pone_val* retval = func(new_world, n, args);
    pone_refcnt_inc(world->universe, retval);
    pone_mortalize(world, retval); // refcnt-- it in upper scope
    va_end(args);

    pone_destroy_world(new_world);

    return retval;
}

