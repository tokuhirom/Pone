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
    printf("world new: %x\n", world);
#endif

    world->universe = universe;

    world->lex = pone_lex_new(world, NULL);
    assert(pone_type(world->lex) == PONE_LEX);

    world->err_handlers = pone_malloc(universe, sizeof(jmp_buf)*PONE_ERR_HANDLERS_INIT);
    world->err_handler_lexs = pone_malloc(universe, sizeof(pone_world*)*PONE_ERR_HANDLERS_INIT);
    world->err_handler_idx = 0;
    world->err_handler_max = PONE_ERR_HANDLERS_INIT;

    kv_init(world->tmpstack);
    kv_init(world->savestack);

    pone_gc_log(world->universe, "[pone gc] create new world %p\n", world);

    pone_world_list_append(universe, world);

    return world;
}

// This routine needs GVL
void pone_world_free(pone_world* world) {
    kv_destroy(world->tmpstack);
    kv_destroy(world->savestack);

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

