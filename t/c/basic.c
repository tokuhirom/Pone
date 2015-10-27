#include "pone.h"

int main(int argc, char** argv) {
    pone_universe* universe = pone_universe_init();
    pone_world* world = pone_new_world(universe);

    pone_savetmps(world);
    pone_push_scope(world);

    pone_val* iv = pone_mortalize(world, pone_new_int(world->universe, 4963));
    pone_builtin_say(world, iv);

    {
        pone_val* iv1 = pone_mortalize(world, pone_new_int(world->universe, 4963));
        pone_val* iv2 = pone_mortalize(world, pone_new_int(world->universe, 5963));
        pone_val* result = pone_add(world, iv1, iv2);
        pone_builtin_say(world, result);
    }

    {
        pone_val* iv1 = pone_mortalize(world, pone_new_int(world->universe, 4649));
        pone_val* iv2 = pone_mortalize(world, pone_new_int(world->universe, 5963));
        pone_val* result = pone_subtract(world, iv1, iv2);
        pone_builtin_say(world, result);
    }

    pone_val* pv = pone_mortalize(world, pone_new_str(world->universe, "Hello, world!", strlen("Hello, world!")));
    pone_builtin_say(world, pv);

    pone_builtin_say(world, pone_true());
    pone_builtin_say(world, pone_false());

    pone_val* nv = pone_mortalize(world, pone_new_num(world->universe, 3.14));
    pone_builtin_say(world, nv);

    {
        pone_savetmps(world);
        pone_push_scope(world);

        pone_val* av = pone_mortalize(world, pone_new_ary(world->universe, 3,
            pone_mortalize(world, pone_new_int(world->universe, 6)),
            pone_mortalize(world, pone_new_int(world->universe, 4)),
            pone_mortalize(world, pone_new_int(world->universe, 3))
        ));
        pone_builtin_say(world, pone_builtin_elems(world, av));
        pone_builtin_say(world, pone_ary_at_pos(av, 0));
        pone_builtin_say(world, pone_ary_at_pos(av, 1));
        pone_builtin_say(world, pone_ary_at_pos(av, 2));
        pone_builtin_say(world, pone_ary_at_pos(av, 3));

        pone_freetmps(world);
        pone_pop_scope(world);
    }

    pone_freetmps(world);
    pone_pop_scope(world);

    pone_destroy_world(world);
    pone_universe_destroy(universe);
}
