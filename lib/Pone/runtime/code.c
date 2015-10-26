#include "pone.h" /* PONE_INC */

pone_val* pone_code_new(pone_world* world, pone_funcptr_t func) {
    world->lex->refcnt++;

    pone_code* cv = (pone_code*)pone_malloc(world, sizeof(pone_code));
    cv->refcnt = 1;
    cv->type = PONE_CODE;
    cv->func = func;
    cv->lex = world->lex;

    // pone_lex_refcnt_inc(world, world->lex);

    return (pone_val*)cv;
}

void pone_code_free(pone_world* world, pone_val* v) {
#ifdef TRACE_CODE
    printf("pone_code_free: %x code:%x\n", world, v);
#endif
    assert(v->type == PONE_CODE);
    pone_code* cv = (pone_code*)v;

    // pone_lex_refcnt_dec(world, cv->lex);
}

pone_val* pone_code_call(pone_world* world, pone_val* code, int n, ...) {
    assert(code->type == PONE_CODE);

    pone_code* cv = (pone_code*)code;
    assert(cv->lex != NULL);
    pone_world* new_world = pone_new_world_from_world(world, cv->lex);

    va_list args;
    va_start(args, n);
    pone_funcptr_t func = cv->func;
    pone_val* retval = func(new_world, n, args);
    pone_refcnt_inc(world, retval);
    pone_mortalize(world, retval); // refcnt-- it in upper scope
    va_end(args);

    pone_destroy_world(new_world);

    return retval;
}

