#include "pone.h" /* PONE_INC */

void* pone_malloc(pone_universe* universe, size_t size) {
    void* p = malloc(size);
    if (!p) {
        fprintf(stderr, "Cannot allocate memory\n");
        exit(1);
    }
    memset(p, 0, size);
    return p;
}

pone_val* pone_obj_alloc(pone_world* world, pone_t type) {
    pone_universe* universe = world->universe;

    assert(universe);
    assert(universe->arena_last != NULL);
    pone_val* val;

#ifdef STRESS_GC
    pone_send_private_sig(PONE_SIG_GC);
#endif

    GVL_LOCK(universe);

    // check free-ed values
    if (universe->freelist) {
        // reuse it.
        val = universe->freelist;
        universe->freelist = universe->freelist->as.free.next;
        val->as.basic.flags = 0;
    } else {
        // there is no free-ed value.
        // then, use value from arena.
        if (universe->arena_last->idx == PONE_ARENA_SIZE) {
            // arena doesn't have an empty slot

            // Run GC.
            pone_send_private_sig(PONE_SIG_GC);

            // alloc new arena
            pone_arena* arena = pone_malloc(universe, sizeof(pone_arena));
            universe->arena_last->next = arena;
            universe->arena_last = arena;
            val = &(arena->values[arena->idx++]);
        } else {
            // use last arena entry
            val = &(universe->arena_last->values[universe->arena_last->idx++]);
        }
    }

    val->as.basic.type   = type;

    pone_save_tmp(world, val);

    GVL_UNLOCK(universe);

    return val;
}

// needs GVL(only called from GC).
void pone_val_free(pone_universe* universe, pone_val* p) {
#ifndef NDEBUG
    // clear val's memory for debugging
    memset(p, 0, sizeof(pone_val));
#endif
#ifndef NO_REUSE
    p->as.free.next = universe->freelist;
    universe->freelist = p;
#endif
}

void pone_free(pone_universe* universe, void* p) {
    free(p);
}

char* pone_strdup(pone_world* world, const char* src, size_t size) {
    char* p = (char*)malloc(size+1);
    if (!p) {
        fprintf(stderr, "Cannot allocate memory\n");
        exit(1);
    }
    memcpy(p, src, size);
    p[size] = '\0';
    return p;
}


