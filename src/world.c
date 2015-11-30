#include "pone.h" /* PONE_INC */
#include "kvec.h"
#include "pone_val_vec.h"
#include "pone_opaque.h"

#define PONE_ERR_HANDLERS_INIT 10

#ifdef WORLD_DEBUG
static inline void pone_world_dump(pone_universe* universe) {
    pone_world* world = universe->world_head;
    if (world) {
        while (world) {
            WORLD_TRACE("%p prev:%p next:%p", world, world->prev, world->next);
            assert(world != world->next);
            world = world->next;
        }
    } else {
        WORLD_TRACE("no world");
    }
}
#endif

pone_world* pone_world_new(pone_universe* universe) {
    assert(universe);

    // we can't use pone_malloc yet.
    pone_world* world = (pone_world*)malloc(sizeof(pone_world));
    if (!world) {
        fprintf(stderr, "Cannot make world\n");
        exit(1);
    }

    memset(world, 0, sizeof(pone_world));
    world->errvar = pone_nil();

    WORLD_TRACE("world new: %p", world);

    world->universe = universe;

    world->arena_last = world->arena_head = malloc(sizeof(pone_arena));
    if (!world->arena_last) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    memset(world->arena_last, 0, sizeof(pone_arena));

    world->lex = pone_lex_new(world, NULL);
    assert(pone_type(world->lex) == PONE_LEX);

    world->err_handlers = pone_malloc(universe, sizeof(jmp_buf) * PONE_ERR_HANDLERS_INIT);
    world->err_handler_lexs = pone_malloc(universe, sizeof(pone_world*) * PONE_ERR_HANDLERS_INIT);
    world->err_handler_idx = 0;
    world->err_handler_max = PONE_ERR_HANDLERS_INIT;

    world->tmpstack.a = NULL;
    world->tmpstack.n = 0;
    world->tmpstack.m = 0;

    world->savestack.a = NULL;
    world->savestack.n = 0;
    world->savestack.m = 0;

    CHECK_PTHREAD(pthread_mutex_init(&(world->mutex), NULL));
    CHECK_PTHREAD(pthread_cond_init(&(world->cond), NULL));

    pone_gc_log(world->universe, "[pone gc] create new world %p\n", world);

    return world;
}

// Clear the world before reuse.
void pone_world_release(pone_world* world) {
    // rewind tmpstack
    world->tmpstack.n = 0;

    // rewind savestack
    world->savestack.n = 0;

    // remove code
    world->code = NULL;

    // run gc before reuse
    if (world->gc_requested) {
        pone_gc_run(world);
    }
}

void pone_world_free(pone_world* world) {
    GC_TRACE("freeing world! %p", world);

#ifndef NDEBUG
    world->freelist = NULL;
    world->tmpstack.n = 0;
    world->tmpstack.m = 0;
    world->savestack.n = 0;
    world->savestack.m = 0;
#endif

    pone_arena* arena = world->arena_head;
    while (arena) {
        for (pone_int_t i = 0; i < arena->idx; ++i) {
            pone_val* val = &(arena->values[i]);
            if (!pone_alive(val)) {
                continue;
            }

            switch (pone_type(val)) {
            case PONE_STRING:
                pone_str_free(world, val);
                break;
            case PONE_ARRAY:
                pone_ary_free(world, val);
                break;
            case PONE_MAP:
                pone_map_free(world, val);
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
        pone_arena* next = arena->next;
        pone_free(world->universe, arena);
        arena = next;
    }

    pone_gc_log(world->universe, "[pone gc] freeing world %p\n", world);

    CHECK_PTHREAD(pthread_mutex_destroy(&(world->mutex)));
    CHECK_PTHREAD(pthread_cond_destroy(&(world->cond)));
    free(world->tmpstack.a);
    free(world->savestack.a);
    pone_free(world->universe, world->err_handler_lexs);
    pone_free(world->universe, world->err_handlers);
    pone_free(world->universe, world);
}

void pone_world_set_errno(pone_world* world) {
    world->errsv = errno;
}

void pone_world_mark(pone_world* world) {
    pone_gc_log(world->universe, "[pone gc] mark world %p\n", world);

    if (world->lex) {
        pone_gc_mark_value(world->lex);
    }
    if (world->errvar) {
        pone_gc_mark_value(world->errvar);
    }
    if (world->stacktrace) {
        pone_gc_mark_value(world->stacktrace);
    }

    if (world->code) {
        pone_gc_mark_value(world->code);
    }

    for (pone_int_t i = 0; i < world->err_handler_idx; ++i) {
        pone_gc_mark_value(world->err_handler_lexs[i]);
    }

    // mark tmp stack
    for (size_t i = 0; i < kv_size(world->tmpstack); ++i) {
        pone_gc_mark_value(kv_A(world->tmpstack, i));
    }
}
