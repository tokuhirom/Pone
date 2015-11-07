#include "pone.h"
#include "rockre.h"

static void run_tests7(pone_world* world) {
    rockre_regexp* re = rockre_compile(world->universe->rockre, "h((o))ge", 8);
    rockre_region* region = rockre_region_new(rockre);
    if (rockre_partial_match(rockre, re, region, "hoge", 4)) {
        printf("matched\n");
    }
    for (int i=0; i< region->num_regs; ++i) {
        printf("%d beg:%d end:%d\n", i, region->beg[i], region->end[i]);
    }
}

// -----------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    pone_universe* universe = pone_universe_init();
    pone_world* world = pone_world_new(universe);

    if (setjmp(universe->err_handlers[0])) {
        world->universe = universe; // magical trash
        pone_universe_default_err_handler(world);
    } else {
        pone_savetmps(world);
        pone_push_scope(world);

        run_tests7(world);

        pone_freetmps(world);
        pone_pop_scope(world);

        pone_destroy_world(world);
        pone_universe_destroy(universe);
    }
}

