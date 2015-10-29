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

pone_val* pone_obj_alloc(pone_universe* universe, pone_t type) {
    assert(universe);
    assert(universe->arena_last != NULL);
    pone_val* val;

    // check free-ed values
    if (universe->freelist) {
        // reuse it.
        val = universe->freelist;
        universe->freelist = universe->freelist->as.free.next;
#ifndef NDEBUG
        // clear val's memory for debugging
        memset(val, 0, sizeof(pone_val));
#endif
    } else {
        // there is no free-ed value.
        // then, use value from arena.
        if (universe->arena_last->idx == PONE_ARENA_SIZE) {
            // arena doesn't have an empty slot
            pone_arena* arena = pone_malloc(universe, sizeof(pone_arena));
            universe->arena_last->next = arena;
            universe->arena_last = arena;
            val = &(arena->values[arena->idx++]);
        } else {
            // use last arena entry
            val = &(universe->arena_last->values[universe->arena_last->idx++]);
        }
    }

    val->as.basic.refcnt = 1;
    val->as.basic.type   = type;
    return val;
}

void pone_val_free(pone_universe* universe, pone_val* p) {
    p->as.free.next = universe->freelist;
    universe->freelist = p;
}

void pone_free(pone_universe* universe, void* p) {
    free(p);
}

char* pone_strdup(pone_universe* universe, const char* src, size_t size) {
    char* p = (char*)malloc(size+1);
    if (!p) {
        fprintf(stderr, "Cannot allocate memory\n");
        exit(1);
    }
    memcpy(p, src, size);
    p[size] = '\0';
    return p;
}

inline void pone_refcnt_inc(pone_universe* universe, pone_val* val) {
#ifdef TRACE_REFCNT
    printf("pone_refcnt_inc: universe:%X val:%X\n", universe, val);
#endif
    assert(val != NULL);

    val->as.basic.refcnt++;
}

// decrement reference count
inline void pone_refcnt_dec(pone_universe* universe, pone_val* val) {
#ifdef TRACE_REFCNT
    printf("refcnt_dec: %X refcnt:%d type:%s\n", val, val->refcnt, pone_what_str_c(val));
#endif

    if (val->as.basic.flags & PONE_FLAGS_GLOBAL) {
        return;
    }

    assert(val != NULL);
#ifndef NDEBUG
    if (val->as.basic.refcnt <= 0) { 
        // pone_dd(universe, val);
        assert(val->as.basic.refcnt > 0);
    }
#endif

    val->as.basic.refcnt--;
    if (val->as.basic.refcnt == 0) {
        switch (pone_type(val)) {
        case PONE_STRING:
            pone_str_free(universe, val);
            break;
        case PONE_ARRAY:
            pone_ary_free(universe, val);
            break;
        case PONE_HASH:
            pone_hash_free(universe, val);
            break;
        case PONE_CODE:
            pone_code_free(universe, val);
            break;
        case PONE_OBJ:
            pone_obj_free(universe, val);
            break;
        case PONE_INT: // don't need to free heap
        case PONE_NUM:
            break;
        case PONE_NIL:
        case PONE_BOOL:
            abort(); // should not reach here.
        }
#ifdef TRACE_REFCNT
        memset(val, 0, sizeof(pone_val));
#endif
        pone_val_free(universe, val);
    }
}

