#include "all.h"

int main(int argc, char** argv) {
    pone_world* world = pone_new_world();

    pone_enter(world);

    {
        pone_enter(world);

        pone_val* av = pone_mortalize(world, pone_new_hash(world, 4,
            pone_mortalize(world, pone_new_int(world, 6)),
            pone_mortalize(world, pone_new_int(world, 4)),
            pone_mortalize(world, pone_new_int(world, 3)),
            pone_mortalize(world, pone_new_int(world, 8))
        ));
        pone_builtin_say(world, pone_builtin_elems(world, av));

        pone_leave(world);
    }

    pone_leave(world);

    pone_destroy_world(world);
}
