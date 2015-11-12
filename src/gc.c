#include "pone.h"

// TODO use bitmap gc
void pone_gc_mark_value(pone_val* val) {
    assert(val);

    if (pone_flags(val) & PONE_FLAGS_GC_MARK) {
        return; // already marked.
    }
    val->as.basic.flags |= PONE_FLAGS_GC_MARK;

    switch (pone_type(val)) {
    case PONE_NIL:
    case PONE_INT:
    case PONE_NUM:
    case PONE_BOOL:
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

static void pone_gc_mark(pone_universe* universe) {
    pone_gc_mark_value(pone_nil());
    pone_gc_mark_value(pone_true());
    pone_gc_mark_value(pone_false());
    pone_universe_mark(universe);
}

static void pone_gc_collect(pone_universe* universe) {
    pone_arena* arena = universe->arena_head;
    while (arena) {
        for (pone_int_t i=0; i< arena->idx; ++i) {
            pone_val* val = &(arena->values[i]);
            if (pone_type(val) == 0) { // free-ed
                continue;
            }

            if (pone_flags(val) & PONE_FLAGS_GC_MARK) {
                // marked.
                // remove marked flag.
                val->as.basic.flags ^= PONE_FLAGS_GC_MARK;
            } else {
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
                    continue;
                    abort(); // should not reach here.
                case PONE_LEX:
                    pone_lex_free(universe, val);
                    break;
                }
                pone_val_free(universe, val);
            }
        }
        arena = arena->next;
    }
}

void pone_gc_run(pone_universe* universe) {
    pone_gc_log(universe, "[pone gc] starting gc\n");

    pone_gc_mark(universe);
    pone_gc_log(universe, "[pone gc] finished marking phase\n");
    pone_gc_collect(universe);

    pone_gc_log(universe, "[pone gc] finished gc\n");
}

// got PONE_SIG_GC private gc
static pone_val* meth_gc_got_sig(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);

    pone_gc_run(world->universe);

    return pone_nil();
}

void pone_gc_init(pone_universe* universe) {
    pone_val* cb = pone_code_new_c(universe, meth_gc_got_sig);
    universe->signal_handlers[PONE_SIG_GC] = cb;
}

