#include "pone.h"

int main(int argc, char** argv) {
    pone_universe* universe = pone_universe_init();
    pone_world* world = pone_new_world(universe);

    pone_savetmps(world);
    pone_push_scope(world);

    {
        pone_savetmps(world);
        pone_push_scope(world);

        pone_val* av = pone_mortalize(world, pone_new_hash(world, 4,
            pone_mortalize(world, pone_new_int(world, 6)),
            pone_mortalize(world, pone_new_int(world, 4)),
            pone_mortalize(world, pone_new_int(world, 3)),
            pone_mortalize(world, pone_new_int(world, 8))
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
