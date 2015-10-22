#include "pone.h" /* PONE_INC */

pone_world* pone_new_world() {
    // we can't use pone_malloc yet.
    pone_world* world = malloc(sizeof(pone_world));
    if (!world) {
        fprintf(stderr, "Cannot make world\n");
        exit(1);
    }
    memset(world, 0, sizeof(pone_world));

    world->savestack = malloc(sizeof(size_t*) * 64);
    if (!world->savestack) {
        fprintf(stderr, "Cannot make world\n");
        exit(1);
    }
    world->savestack_max = 64;

    world->tmpstack = malloc(sizeof(size_t*) * 64);
    if (!world->tmpstack) {
        fprintf(stderr, "Cannot make world\n");
        exit(1);
    }
    world->tmpstack_max = 64;

    world->lex = malloc(sizeof(lex_entry));
    if (!world->lex) {
        fprintf(stderr, "Cannot make world\n");
        exit(1);
    }
    world->lex->map = kh_init(str);
    world->lex->parent = NULL;

    return world;
}

void pone_destroy_world(pone_world* world) {
    {
        const char* k;
        pone_val* v;
        kh_foreach(world->lex->map, k, v, {
            pone_refcnt_dec(world, v);
        });
        kh_destroy(str, world->lex->map);
        free(world->lex);
    }

    free(world->savestack);
    free(world->tmpstack);
    free(world);
}

