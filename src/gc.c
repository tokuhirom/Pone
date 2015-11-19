#include "pone.h"
#include <errno.h>

void pone_gc_mark_value(pone_val* val) {
    assert(val);

    if (pone_flags(val) & PONE_FLAGS_GC_MARK) {
        return; // already marked.
    }
    val->as.basic.flags |= PONE_FLAGS_GC_MARK;
    assert(pone_flags(val) & PONE_FLAGS_GC_MARK);

    switch (pone_type(val)) {
    case PONE_NIL:
    case PONE_INT:
    case PONE_NUM:
    case PONE_BOOL:
    case PONE_OPAQUE:
        break; // primitive values.
    case PONE_STRING:
        pone_str_mark(val);
        break;
    case PONE_ARRAY:
        pone_ary_mark(val);
        break;
    case PONE_HASH:
        pone_hash_mark(val);
        break;
    case PONE_CODE:
        pone_code_mark(val);
        break;
    case PONE_OBJ:
        pone_obj_mark(val);
        break;
    case PONE_LEX:
        pone_lex_mark(val);
        break;
    }
}

static void pone_gc_mark(pone_world* world) {
    pone_universe_mark(world->universe);
    pone_world_mark(world);
}

static void pone_gc_collect(pone_world* world) {
#ifndef NDEBUG
    pone_int_t freed_cnt = 0;
#endif
    pone_arena* arena = world->arena_head;
    while (arena) {
        for (pone_int_t i=0; i< arena->idx; ++i) {
            pone_val* val = &(arena->values[i]);
            if (pone_type(val) == 0) { // free-ed
                continue;
            }

            if (pone_flags(val) & PONE_FLAGS_GC_MARK) {
                // marked.
                // remove marked flag.
                GC_TRACE("marked obj: %p %s", val, pone_what_str_c(val));
                val->as.basic.flags ^= PONE_FLAGS_GC_MARK;
            } else {
#ifndef NDEBUG
                freed_cnt++;
#endif
                GC_TRACE("free: %p %s", val, pone_what_str_c(val));
                switch (pone_type(val)) {
                case PONE_STRING:
                    pone_str_free(world, val);
                    break;
                case PONE_ARRAY:
                    pone_ary_free(world, val);
                    break;
                case PONE_HASH:
                    pone_hash_free(world, val);
                    break;
                case PONE_OBJ:
                    pone_obj_free(world, val);
                    break;
                case PONE_CODE:
                case PONE_INT: // don't need to free heap
                case PONE_NUM:
                    break;
                case PONE_OPAQUE:
                    pone_opaque_free(world, val);
                    break;
                case PONE_NIL:
                case PONE_BOOL:
                    continue;
                    abort(); // should not reach here.
                case PONE_LEX:
                    pone_lex_free(world, val);
                    break;
                }
                pone_val_free(world, val);
            }
        }
        arena = arena->next;
    }
#ifndef NDEBUG
    pone_gc_log(world->universe, "[pone gc] freed %ld\n", freed_cnt);
#endif
}

void pone_gc_run(pone_world* world) {
    pone_gc_log(world->universe, "[pone gc] starting gc\n");

    pone_gc_mark(world);

    pone_gc_log(world->universe, "[pone gc] finished marking phase\n");
    pone_gc_collect(world);

    world->gc_requested = false;

    pone_gc_log(world->universe, "[pone gc] finished gc\n");
}

PONE_FUNC(meth_gc_request) {
    PONE_ARG("GC#request", "");

    world->gc_requested = true;

    return pone_nil();
}

void pone_gc_init(pone_world* world) {
    pone_universe* universe = world->universe;
    pone_val* gc = pone_class_new(world, "GC", strlen("GC"));
    PONE_REG_METHOD(gc, "request", meth_gc_request);
    pone_class_compose(world, gc);
    pone_universe_set_global(universe, "GC", gc);
}

