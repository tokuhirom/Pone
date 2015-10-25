#include "pone_all.h"

int main(int argc, char** argv) {
    pone_world* world = pone_new_world();

    pone_savetmps(world);
    pone_push_scope(world);

    assert(world->tmpstack_floor == 0);
    assert(world->tmpstack_idx == 0);

    pone_mortalize(world, pone_new_int(world, 6));

    {
        pone_savetmps(world);
        pone_push_scope(world);

        assert(world->tmpstack_floor == 1);
        assert(world->tmpstack_idx == 1);

        pone_mortalize(world, pone_new_int(world, 6));

        assert(world->tmpstack_floor == 1);
        assert(world->tmpstack_idx == 2);

        pone_freetmps(world);
        pone_pop_scope(world);

        assert(world->tmpstack_floor == 0);
        assert(world->tmpstack_idx == 1);
    }

    for (int i=0; i<1000; ++i) {
        pone_mortalize(world, pone_new_int(world, 6));
    }

    {
        pone_savetmps(world);
        pone_push_scope(world);

        for (int i=0; i<1000; ++i) {
            pone_mortalize(world, pone_new_int(world, 6));
        }

        pone_freetmps(world);
        pone_pop_scope(world);
    }

    {
        for (int i=0; i<1000; ++i) {
            pone_savetmps(world);
            pone_push_scope(world);
        }
        for (int i=0; i<1000; ++i) {
            pone_freetmps(world);
            pone_pop_scope(world);
        }
    }

    pone_freetmps(world);
    pone_pop_scope(world);

    pone_destroy_world(world);
}

