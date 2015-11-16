#include "pone.h" /* PONE_INC */

pone_val* pone_lex_new(pone_world* world, pone_val* parent) {
    GC_LOCK(world->universe);
    pone_val* lex = pone_obj_alloc(world, PONE_LEX);
    assert(lex->as.basic.type == PONE_LEX);
#ifdef TRACE_LEX
    printf("[pone lex] pone_lex_new: %p lex:%p\n", world, lex);
#endif
    lex->as.lex.map = kh_init(str);
    lex->as.lex.parent = parent;
    lex->as.lex.thread_id = pthread_self();
    assert(lex->as.basic.type == PONE_LEX);
    GC_UNLOCK(world->universe);
    return lex;
}

void pone_lex_mark(pone_val* lex) {
#ifdef TRACE_LEX
    printf("lex mark %p flags:%d\n", lex, lex->as.lex.flags);
#endif

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

    GC_LOCK(world->universe);
    if (world->savestack.n == world->savestack.m) {
        world->savestack.m = world->savestack.m ? world->savestack.m<<1 : 2;
        world->savestack.a = (size_t*)realloc(world->savestack.a,
                sizeof(size_t) * world->savestack.m);
        PONE_ALLOC_CHECK(world->savestack.a);
    }
    world->savestack.a[world->savestack.n++] = world->tmpstack.n;
    GC_UNLOCK(world->universe);
}

void pone_lex_free(pone_universe* universe, pone_val* val) {
    GC_TRACE("pone_lex_free: %p lex:%p\n", universe, val);
    kh_destroy(str, val->as.lex.map);
}

void pone_pop_scope(pone_world* world) {
#ifdef TRACE_SCOPE
    printf("pone_pop_scope: %x\n", world);
#endif
#ifndef NDEBUG
    if (pone_type(world->lex) != PONE_LEX) {
        fprintf(stderr, "[BUG] world->lex(%p) isn't lex: %d. world:%p\n", world->lex, pone_type(world->lex), world);
        abort();
    }
#endif
    world->lex = world->lex->as.lex.parent;
#ifndef NDEBUG
    if (pone_type(world->lex) != PONE_LEX) {
        fprintf(stderr, "[BUG] bad parent! world->lex(%p) isn't lex: %d. world:%p\n", world->lex, pone_type(world->lex), world);
        abort();
    }
#endif

    GC_LOCK(world->universe);
    world->tmpstack.n = world->savestack.a[--(world->savestack.n)];
    GC_UNLOCK(world->universe);
}

// This function may called from section that protected by GC_LOCK.
pone_val* pone_save_tmp(pone_world* world, pone_val* val) {
    // ASSERT_GC_LOCK(world->universe);
    if (world->tmpstack.n == world->tmpstack.m) {
        world->tmpstack.m = world->tmpstack.m ? world->tmpstack.m<<1 : 2;
        world->tmpstack.a = (pone_val**)realloc(world->tmpstack.a,
                sizeof(pone_val*)*world->tmpstack.m);
        PONE_ALLOC_CHECK(world->tmpstack.a);
    }
    world->tmpstack.a[world->tmpstack.n++] = val;
    return val;
}

