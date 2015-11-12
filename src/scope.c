#include "pone.h" /* PONE_INC */

pone_val* pone_lex_new(pone_world* world, pone_val* parent) {
    pone_lex_t* lex = (pone_lex_t*)pone_obj_alloc(world->universe, PONE_LEX);
#ifdef TRACE_LEX
    printf("pone_lex_new: %x lex:%x\n", world, lex);
#endif
    lex->map = kh_init(str);
    lex->parent = parent;
    return (pone_val*)lex;
}

void pone_lex_mark(pone_val* lex) {
    const char* k;
    pone_val* v;
    kh_foreach(lex->as.lex.map, k, v, {
        pone_gc_mark_value(v);
    });
    if (lex->as.lex.parent) {
        pone_gc_mark_value((pone_val*)lex->as.lex.parent);
    }
}

// for debug
void pone_lex_dump(pone_val* lex) {
    printf("--- LEX DUMP ---\n");
    int i=0;
    while (lex) {
        printf("--- %d\n", i);

        const char* k;
        pone_val* v;
        kh_foreach(lex->as.lex.map, k, v, {
            printf("  %s: %p\n", k, v);
        });

        lex = lex->as.lex.parent;
        ++i;
    }
    printf("--- /LEX DUMP ---\n");
}

void pone_push_scope(pone_world* world) {
#ifdef TRACE_SCOPE
    printf("pone_push_scope: %x\n", world);
#endif
    // create new lex scope
    world->lex = pone_lex_new(world, world->lex);
    assert(pone_type(world->lex) == PONE_LEX);
}

void pone_lex_free(pone_universe* universe, pone_val* val) {
#ifdef TRACE_LEX
    printf("pone_lex_free: %p lex:%p\n", universe, val);
#endif
    kh_destroy(str, val->as.lex.map);
}

void pone_pop_scope(pone_world* world) {
#ifdef TRACE_SCOPE
    printf("pone_pop_scope: %x\n", world);
#endif
    world->lex = world->lex->as.lex.parent;
    assert(pone_type(world->lex) == PONE_LEX);
}

