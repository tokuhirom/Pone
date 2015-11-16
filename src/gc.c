#include "pone.h"
#include <errno.h>

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
    pone_world* world = universe->world_head;
    while (world) {
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
                    GC_TRACE("free: %p", val);
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
                    pone_val_free(world, val);
                }
            }
            arena = arena->next;
        }
        world = world->next;
    }
}

void pone_gc_run(pone_universe* universe) {
    pone_gc_log(universe, "[pone gc] starting gc\n");

    CHECK_PTHREAD(pthread_rwlock_wrlock(&(universe->gc_rwlock)));
    pone_gc_mark(universe);
    CHECK_PTHREAD(pthread_rwlock_unlock(&(universe->gc_rwlock)));

    pone_gc_log(universe, "[pone gc] finished marking phase\n");
    pone_gc_collect(universe);

    pone_gc_log(universe, "[pone gc] finished gc\n");

    // TODO we should unlock GC lock after marking phase. Since sweeping phase should only
    // touch objects, that aren't reachable.
}

void pone_gc_request(pone_universe* universe) {
    if (universe->gc_requested) {
        pone_gc_log(universe, "gc is already requested\n");
        return;
    }

    pone_gc_log(universe, "pone_gc_request\n");
    CHECK_PTHREAD(pthread_mutex_lock(&(universe->gc_thread_mutex)));
    universe->gc_requested = true;
    CHECK_PTHREAD(pthread_cond_signal(&(universe->gc_cond)));
    CHECK_PTHREAD(pthread_mutex_unlock(&(universe->gc_thread_mutex)));
    pthread_yield();
}

static pone_val* meth_gc_run(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);

    pone_gc_request(world->universe);

    return pone_nil();
}

static void* gc_thread(void* p) {
    pone_universe* universe = (pone_universe*)p;

    // universe.c will cancel at process termination
    int oldstate;
    CHECK_PTHREAD(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE|PTHREAD_CANCEL_ASYNCHRONOUS, &oldstate));

    CHECK_PTHREAD(pthread_mutex_lock(&(universe->gc_thread_mutex)));

    while (!universe->in_global_destruction) {
        GC_TRACE("GC thread waiting GC request...");
        CHECK_PTHREAD(pthread_cond_wait(&(universe->gc_cond), &(universe->gc_thread_mutex)));
        GC_TRACE("GC thread got gc request");
        if (universe->gc_requested) {
            pone_gc_run(universe);
            universe->gc_requested = false;
        }
    }
    CHECK_PTHREAD(pthread_mutex_unlock(&(universe->gc_thread_mutex)));

    return NULL;
}

void pone_gc_init(pone_world* world) {
    pone_universe* universe = world->universe;
    pone_val* gc = pone_class_new(world, "GC", strlen("GC"));
    pone_add_method_c(world, gc, "run", strlen("run"), meth_gc_run);
    pone_class_compose(world, gc);
    pone_universe_set_global(universe, "GC", gc);

    int r;
    if ((r=pthread_create(&(universe->gc_thread), NULL, gc_thread, universe)) != 0) {
        errno=r;
        perror("cannot create gc thread");
    }
}

