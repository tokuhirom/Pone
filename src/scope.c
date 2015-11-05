#include "pone.h" /* PONE_INC */

void pone_savetmps(pone_world* world) {
    if (world->savestack_max==world->savestack_idx) {
        // grow it
        world->savestack_max *= 2;
        size_t* ssp = (size_t*)realloc(world->savestack, sizeof(size_t)*world->savestack_max);
        if (!ssp) {
            pone_throw_str(world, "Cannot allocate memory");
        }
        world->savestack = ssp;
    }

    // save original tmpstack_floor
    world->savestack[world->savestack_idx++] = world->tmpstack_floor;

    // save current tmpstack_idx
    world->tmpstack_floor = world->tmpstack_idx;
}

pone_lex_t* pone_lex_new(pone_world* world, pone_lex_t* parent) {
    pone_lex_t* lex = (pone_lex_t*)pone_obj_alloc(world->universe, PONE_NIL);
#ifdef TRACE_LEX
    printf("pone_lex_new: %x lex:%x\n", world, lex);
#endif
    lex->map = kh_init(str);
    lex->parent = parent;
    if (parent) {
        pone_lex_refcnt_inc(world, parent);
    }
    return lex;
}

void pone_push_scope(pone_world* world) {
#ifdef TRACE_SCOPE
    printf("pone_push_scope: %x\n", world);
#endif

    // create new lex scope
    world->lex = pone_lex_new(world, world->lex);
}

static void pone_lex_free(pone_world* world, pone_lex_t* lex) {
#ifdef TRACE_LEX
    printf("pone_lex_free: %x lex:%x\n", world, lex);
#endif
    {
        const char* k;
        pone_val* v;
        kh_foreach(lex->map, k, v, {
            pone_refcnt_dec(world->universe, v);
        });
    }
    kh_destroy(str, lex->map);
    if (lex->parent) {
        pone_lex_refcnt_dec(world, lex->parent);
    }
    pone_val_free(world->universe, (pone_val*)lex);
}

void pone_lex_refcnt_dec(pone_world* world, pone_lex_t* lex) {
#ifdef TRACE_LEX
    printf("pone_lex_refcnt_dec: %x lex:%x refcnt:%d\n", world, lex, lex->refcnt);
#endif
    assert(lex->refcnt > 0);
    lex->refcnt--;
    if (lex->refcnt == 0) {
        pone_lex_free(world, lex);
    }
}

inline void pone_lex_refcnt_inc(pone_world* world, pone_lex_t* lex) {
#ifdef TRACE_LEX
    printf("pone_lex_refcnt_inc: %x lex:%x refcnt:%d\n", world, lex, lex->refcnt);
#endif
    lex->refcnt++;
}

void pone_pop_scope(pone_world* world) {
#ifdef TRACE_SCOPE
    printf("pone_pop_scope: %x\n", world);
#endif
    pone_lex_t* parent = world->lex->parent;
    pone_lex_refcnt_dec(world, world->lex);
    world->lex = parent;
}

// free mortal values
void pone_freetmps(pone_world* world) {
#ifdef TRACE_REFCNT
    printf("pone_freetmps: world:%x tmpstack_idx:%d savestack_idx:%d\n", world, world->tmpstack_idx, world->savestack_idx);
#endif
    // decrement refcnt for mortalized values
    while (world->tmpstack_idx > world->tmpstack_floor) {
        pone_refcnt_dec(world->universe, world->tmpstack[world->tmpstack_idx-1]);
        --world->tmpstack_idx;
    }

    // restore tmpstack_floor
    world->tmpstack_floor = world->savestack[world->savestack_idx-1];

    // pop tmpstack_floor
    --world->savestack_idx;
}

pone_val* pone_mortalize(pone_world* world, pone_val* val) {
#ifdef TRACE_REFCNT
    printf("pone_mortalize: world:%X val:%X val->type:%s\n", world, val, pone_what_str_c(val));
#endif
    if (world->tmpstack_idx == world->tmpstack_max) {
        world->tmpstack_max *= 2;
        pone_val** ssp = (pone_val**)realloc(world->tmpstack, sizeof(pone_val*)*world->tmpstack_max);
        if (!ssp) {
            pone_throw_str(world, "Cannot allocate memory");
        }
        world->tmpstack = ssp;
    }
    world->tmpstack[world->tmpstack_idx] = val;
    ++world->tmpstack_idx;
    return val;
}

