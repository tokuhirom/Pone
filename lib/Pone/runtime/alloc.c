#include "pone.h" /* PONE_INC */

// TODO: implement memory pool
void* pone_malloc(pone_world* world, size_t size) {
    void* p = malloc(size);
    if (!p) {
        fprintf(stderr, "Cannot allocate memory\n");
        exit(1);
    }
    memset(p, 0, size);
    return p;
}

#define PONE_ERR_HANDLERS_INIT 10

void pone_universe_default_err_handler(pone_universe* universe) {
    assert(universe->errvar);
    pone_val* str = pone_to_str(universe, universe->errvar);
    fwrite("\n!!!!!!!!!orz!!!!!!!!!\n\n", 1, strlen("\n!!!!!!!!!orz!!!!!!!!!\n\n"), stderr);
    fwrite(pone_string_ptr(str), 1, pone_string_len(str), stderr);
    fwrite("\n\n", 1, strlen("\n\n"), stderr);
    exit(1);
}

pone_universe* pone_universe_init() {
    pone_universe*universe = malloc(sizeof(pone_universe));
    if (!universe) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    memset(universe, 0, sizeof(pone_universe));
    universe->arena_last = universe->arena_head = malloc(sizeof(pone_arena));
    if (!universe->arena_last) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    memset(universe->arena_last, 0, sizeof(pone_arena));
    universe->errvar = pone_nil();

    universe->err_handlers = malloc(sizeof(jmp_buf)*PONE_ERR_HANDLERS_INIT);
    if (!universe->err_handlers) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    universe->err_handler_idx = 0;
    universe->err_handler_max = PONE_ERR_HANDLERS_INIT;

    return universe;
}

void pone_universe_destroy(pone_universe* universe) {
    pone_arena* a = universe->arena_head;
    while (a) {
        pone_arena* next = a->next;
        free(a);
        a = next;
    }
    if (universe->errvar) {
        pone_refcnt_dec(universe, universe->errvar);
    }
    free(universe->err_handlers);
    free(universe);
}

pone_val* pone_obj_alloc(pone_world* world, pone_t type) {
    assert(world->universe);
    pone_universe* universe = world->universe;
    assert(world->universe->arena_last != NULL);
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
            pone_arena* arena = pone_malloc(world, sizeof(pone_arena));
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

void pone_obj_free(pone_universe* universe, pone_val* p) {
    p->as.free.next = universe->freelist;
    universe->freelist = p;
}

void pone_free(pone_universe* universe, void* p) {
    free(p);
}

const char* pone_strdup(pone_world* world, const char* src, size_t size) {
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
        pone_obj_free(universe, val);
    }
}

