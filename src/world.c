#include "pone.h" /* PONE_INC */
#include "kvec.h"

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

// This routine needs GVL
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

    world->err_handlers = pone_malloc(universe, sizeof(jmp_buf)*PONE_ERR_HANDLERS_INIT);
    world->err_handler_lexs = pone_malloc(universe, sizeof(pone_world*)*PONE_ERR_HANDLERS_INIT);
    world->err_handler_idx = 0;
    world->err_handler_max = PONE_ERR_HANDLERS_INIT;

    world->tmpstack.a = NULL;
    world->tmpstack.n = 0;
    world->tmpstack.m = 0;

    world->savestack.a = NULL;
    world->savestack.n = 0;
    world->savestack.m = 0;

    pone_val_vec_init(&(world->channels));

    CHECK_PTHREAD(pthread_mutex_init(&(world->mutex), NULL));
    CHECK_PTHREAD(pthread_cond_init(&(world->cond), NULL));

    pone_gc_log(world->universe, "[pone gc] create new world %p\n", world);

    return world;
}

// Clear the world before reuse.
void pone_world_release(pone_world* world) {
    // rewind tmpstack
    world->tmpstack.n=0;

    // rewind savestack
    world->savestack.n=0;

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
    world->tmpstack.n=0;
    world->tmpstack.m=0;
    world->savestack.n=0;
    world->savestack.m=0;
#endif

//  pone_arena* a = world->arena_head;
//  while (a) {
//      pone_arena* next = a->next;
//      free(a);
//      a = next;
//  }

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
    pone_gc_mark_value(world->errvar);

    if (world->code) {
        pone_gc_mark_value(world->code);
    }

    for (pone_int_t i=0; i<world->err_handler_idx; ++i) {
        pone_gc_mark_value(world->err_handler_lexs[i]);
    }

    // mark tmp stack
    for (size_t i=0; i<kv_size(world->tmpstack); ++i) {
        pone_gc_mark_value(kv_A(world->tmpstack,i));
    }

    // captured channels
    for (pone_int_t i=0; i<pone_val_vec_size(&(world->channels)); ++i) {
        pone_val* chan = pone_val_vec_get(&(world->channels), i);
        if (chan->as.basic.type == PONE_OBJ && chan->as.obj.klass == world->universe->class_channel) {
            pone_val* v = pone_val_vec_get(&(world->channels), i);
            // do not mark channel itself. mark channel's queue!
            pone_chan_mark_queue(world, v);
        } else {
            // maybe this value was reused or freed.
            THREAD_TRACE("remove from world->channels %p", chan);
            pone_val_vec_delete(&(world->channels), i);
            i--;
        }
    }
}

