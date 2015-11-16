#include "pone.h" /* PONE_INC */
#include "utlist.h"

#define PONE_ERR_HANDLERS_INIT 10

#ifdef TRACE_WORLD
static inline void pone_world_dump(pone_universe* universe) {
    pone_world* world = universe->world_head;
    if (world) {
        while (world) {
            printf("WWW %p prev:%p next:%p\n", world, world->prev, world->next);
            assert(world != world->next);
            world = world->next;
        }
    } else {
        printf("WWW no world\n");
    }
}
#endif

// This routine needs GVL
static inline void pone_world_list_append(pone_universe *universe, pone_world* world) {
#ifdef TRACE_WORLD
    printf("WWW ADD %p\n", world);
    pone_world_dump(universe);
#endif

#define HEAD (universe->world_head)

    if (HEAD) {
        world->prev = HEAD->prev;
        HEAD->prev->next = world;
        HEAD->prev = world;
        world->next = NULL;
    } else {
        HEAD = world;
        HEAD->prev = HEAD;
        HEAD->next = NULL;
    }

#undef HEAD
}

// This routine needs GVL
static inline void pone_world_list_remove(pone_universe *universe, pone_world* world) {
    DL_DELETE(universe->world_head, world);
}

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

#ifdef TRACE_WORLD
    printf("[pone world] world new: %p\n", world);
#endif

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

    pone_gc_log(world->universe, "[pone gc] create new world %p\n", world);

    UNIVERSE_LOCK(world->universe); // This operation modifies universe's structure.
    pone_world_list_append(universe, world);
    UNIVERSE_UNLOCK(world->universe);

    return world;
}

// This routine needs GVL
void pone_world_free(pone_world* world) {
    GC_TRACE("freeing world! %p\n", world);

    // pass free'd arenas to universe.
    world->arena_head->next = world->universe->freed_arena;
    world->universe->freed_arena = world->arena_head;

    pone_val* v = world->freelist;
    while (v) {
        pone_val* nv = v->as.free.next;
        v->as.free.next = world->universe->freed_freelist;
        world->universe->freed_freelist = v;
        v = nv;
    }

#ifndef NDEBUG
    world->tmpstack.n=0;
    world->tmpstack.m=0;
    world->savestack.n=0;
    world->savestack.m=0;
#endif
    free(world->tmpstack.a);
    free(world->savestack.a);

//  pone_arena* a = world->arena_head;
//  while (a) {
//      pone_arena* next = a->next;
//      free(a);
//      a = next;
//  }

    pone_gc_log(world->universe, "[pone gc] freeing world %p\n", world);
    pone_world_list_remove(world->universe, world);
    pone_free(world->universe, world->err_handler_lexs);
    pone_free(world->universe, world->err_handlers);
    pone_free(world->universe, world);
}

void pone_world_mark(pone_world* world) {
    pone_gc_log(world->universe, "[pone gc] mark world %p\n", world);

    pone_gc_mark_value(world->lex);
    pone_gc_mark_value(world->errvar);

    for (pone_int_t i=0; i<world->err_handler_idx; ++i) {
        pone_gc_mark_value(world->err_handler_lexs[i]);
    }

    // mark tmp stack
    for (size_t i=0; i<kv_size(world->tmpstack); ++i) {
        pone_gc_mark_value(kv_A(world->tmpstack,i));
    }
}

