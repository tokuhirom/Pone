#include "pone.h" /* PONE_INC */

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

static inline void pone_world_list_append(pone_universe *universe, pone_world* world) {
#ifdef TRACE_WORLD
    printf("WWW ADD %p\n", world);
    pone_world_dump(universe);
#endif

    if (universe->world_head) {
        universe->world_head->prev = world;
        world->next = universe->world_head;
    }
    universe->world_head = world;
}

static inline void pone_world_list_remove(pone_universe *universe, pone_world* world) {
    if (world->prev) {
        world->prev->next = world->next;
    }
    if (world->next) {
        world->next->prev = world->prev;
    }
    if (world == universe->world_head) {
        universe->world_head = world->next;
    }
}

pone_world* pone_world_new(pone_universe* universe) {
    assert(universe);

    GVL_LOCK(universe);

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

    world->tmpstack = pone_ary_new(world->universe, 0);

    pone_world_list_append(universe, world);

    return world;
}

void pone_world_free(pone_world* world) {
    pone_world_list_remove(world->universe, world);
    pone_free(world->universe, world->err_handler_lexs);
    pone_free(world->universe, world->err_handlers);
    pone_free(world->universe, world);
}

void pone_world_mark(pone_world* world) {
    world->mark = true;
    pone_lex_mark(world->lex);
    pone_gc_mark_value(world->errvar);

    for (pone_int_t i=0; i<world->err_handler_idx; ++i) {
        pone_lex_mark(world->err_handler_lexs[i]);
    }

    // mark tmp stack
    {
        pone_int_t l = pone_ary_elems(world->tmpstack);
        for (pone_int_t i=0; i<l; i++) {
            pone_int_t k = pone_ary_elems(world->tmpstack->as.ary.a[i]);
            for (pone_int_t j=0; i<k; i++) {
                pone_gc_mark_value(world->tmpstack->as.ary.a[i]->as.ary.a[j]);
            }
        }
    }
}

