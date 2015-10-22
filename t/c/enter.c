#include "pone_all.h"

int main(int argc, char** argv) {
    pone_world* world = pone_new_world();

    pone_enter(world);

    assert(world->tmpstack_floor == 0);
    assert(world->tmpstack_idx == 0);

    pone_mortalize(world, pone_new_int(world, 6));

    {
        pone_enter(world);

        assert(world->tmpstack_floor == 1);
        assert(world->tmpstack_idx == 1);

        pone_mortalize(world, pone_new_int(world, 6));

        assert(world->tmpstack_floor == 1);
        assert(world->tmpstack_idx == 2);

        pone_leave(world);

        assert(world->tmpstack_floor == 0);
        assert(world->tmpstack_idx == 1);
    }

    for (int i=0; i<1000; ++i) {
        pone_mortalize(world, pone_new_int(world, 6));
    }

    {
        pone_enter(world);

        for (int i=0; i<1000; ++i) {
            pone_mortalize(world, pone_new_int(world, 6));
        }

        pone_leave(world);
    }

    {
        for (int i=0; i<1000; ++i) {
            pone_enter(world);
        }
        for (int i=0; i<1000; ++i) {
            pone_leave(world);
        }
    }

    pone_leave(world);

    pone_destroy_world(world);
}

