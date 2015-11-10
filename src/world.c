#include "pone.h" /* PONE_INC */

pone_world* pone_world_new(pone_universe* universe) {
    assert(universe);

    // we can't use pone_malloc yet.
    pone_world* world = (pone_world*)malloc(sizeof(pone_world));
    if (!world) {
        fprintf(stderr, "Cannot make world\n");
        exit(1);
    }
    memset(world, 0, sizeof(pone_world));
    world->refcnt = 1;

    world->savestack = (size_t*)malloc(sizeof(size_t*) * 64);
    if (!world->savestack) {
        fprintf(stderr, "Cannot make world\n");
        exit(1);
    }
    world->savestack_max = 64;

    world->tmpstack = (pone_val**)malloc(sizeof(pone_val*) * 64);
    if (!world->tmpstack) {
        fprintf(stderr, "Cannot make world\n");
        exit(1);
    }
    world->tmpstack_max = 64;

    world->universe = universe;

    world->lex = pone_lex_new(world, NULL);

    return world;
}

pone_world* pone_world_new_from_world(pone_world* world, pone_lex_t* lex) {
#ifdef TRACE_WORLD
    printf("pone_world_new_from_world: %X parent:%X tmpstack_idx:%X\n", world, world->parent, world->tmpstack_idx);
#endif
    // we can't use pone_malloc yet.
    pone_world* new_world = (pone_world*)pone_malloc(world->universe, sizeof(pone_world));
    new_world->universe = world->universe;
    new_world->parent = world;
    new_world->refcnt = 1;
    pone_world_refcnt_inc(world);

    new_world->savestack = (size_t*)pone_malloc(world->universe, sizeof(size_t*) * 64);
    new_world->savestack_max = 64;

    new_world->tmpstack = (pone_val**)pone_malloc(world->universe, sizeof(pone_val*) * 64);
    new_world->tmpstack_max = 64;
    new_world->tmpstack_idx = 0;
    new_world->tmpstack_floor = 0;

    new_world->lex = pone_lex_new(world, lex);
    new_world->orig_lex = lex;

    pone_savetmps(new_world);
    pone_push_scope(new_world);

    return new_world;
}

void pone_world_refcnt_inc(pone_world* world) {
    world->refcnt++;
}

void pone_world_refcnt_dec(pone_world* world) {
    world->refcnt--;

    if (world->refcnt == 0) {
#ifdef TRACE_WORLD
        printf("destroy_world: %X parent:%X tmpstack_idx:%X\n", world, world->parent, world->tmpstack_idx);
#endif
        if (world->parent) {
            pone_world_refcnt_dec(world->parent);
        }

        while (world->tmpstack_idx > 0) {
            pone_freetmps(world);
        }

        if (world->orig_lex != NULL) {
            while (world->lex != world->orig_lex) {
                pone_pop_scope(world);
            }
        }

        pone_lex_refcnt_dec(world, world->lex);

        free(world->savestack);
        free(world->tmpstack);
        free(world);
    }
}

