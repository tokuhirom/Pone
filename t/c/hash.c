#include "pone.h"

int main(int argc, char** argv) {
    pone_universe* universe = pone_universe_init();
    pone_world* world = pone_world_new(universe);

    pone_savetmps(world);
    pone_push_scope(world);

    {
        pone_savetmps(world);
        pone_push_scope(world);

        pone_val* av = pone_mortalize(world, pone_hash_puts(
            world,
            pone_hash_new(world->universe), 4,
            pone_mortalize(world, pone_int_new(world->universe, 6)),
            pone_mortalize(world, pone_int_new(world->universe, 4)),
            pone_mortalize(world, pone_int_new(world->universe, 3)),
            pone_mortalize(world, pone_int_new(world->universe, 8))
        ));
        pone_builtin_say(world, pone_builtin_elems(world, av));

        pone_freetmps(world);
        pone_pop_scope(world);
    }

    pone_freetmps(world);
    pone_pop_scope(world);

    pone_destroy_world(world);
    pone_universe_destroy(universe);
}
