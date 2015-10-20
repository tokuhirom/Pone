#include "../../lib/Pone/runtime.c"

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

    pone_leave(world);

    pone_destroy_world(world);
}

