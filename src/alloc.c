#include "pone.h" /* PONE_INC */

void* pone_malloc(pone_world* world, size_t size) {
    void* p = malloc(size);
    if (!p) {
        fprintf(stderr, "Cannot allocate memory\n");
        exit(1);
    }
    return p;
}

// Caller must get GC_RD_LOCK.
pone_val* pone_obj_alloc(pone_world* world, pone_t type) {
    pone_universe* universe = world->universe;

    assert(universe);
    assert(world->arena_last != NULL);
    pone_val* val;

#ifdef STRESS_GC
    pone_gc_log(world->universe, "stress gc\n");
    world->gc_requested = true;
#endif

    // check free-ed values
    if (world->freelist) {
        // reuse it.
        val = world->freelist;
        world->freelist = world->freelist->as.free.next;
    } else {
        // there is no free-ed value.
        // then, use value from arena.
        if (world->arena_last->idx == PONE_ARENA_SIZE) {
            // arena doesn't have an empty slot

            // Run GC.
            pone_gc_log(world->universe, "there's no empty slot in the arena %p\n", world->arena_last);
            world->gc_requested = true;

            // alloc new arena
            pone_arena* arena = pone_malloc(world, sizeof(pone_arena));
            arena->next = NULL;
            arena->idx = 0;
            world->arena_last->next = arena;
            world->arena_last = arena;
            val = &(arena->values[arena->idx]);
            arena->idx++;
        } else {
            // use last arena entry
            val = &(world->arena_last->values[world->arena_last->idx++]);
        }
    }

    val->as.basic.flags = 0;
    val->as.basic.type = type;

    pone_save_tmp(world, val);

    return val;
}

// needs GVL(only called from GC).
// GC must get ALLOC_LOCK.
void pone_val_free(pone_world* world, pone_val* p) {
#ifndef NDEBUG
    // clear val's memory for debugging
    memset(p, 0, sizeof(pone_val));
#endif
#ifndef NO_REUSE
    p->as.free.next = world->freelist;
    world->freelist = p;
#endif
}

void pone_free(pone_world* world, void* p) {
    free(p);
}

char* pone_strdup(pone_world* world, const char* src, size_t size) {
    char* p = (char*)malloc(size + 1);
    if (!p) {
        fprintf(stderr, "Cannot allocate memory\n");
        exit(1);
    }
    memcpy(p, src, size);
    p[size] = '\0';
    return p;
}
