#include "pone.h"

int main(int argc, char** argv) {
    pone_init();

    pone_universe* universe = pone_universe_init();
    pone_world* world = pone_world_new(universe);

    pone_savetmps(world);
    pone_push_scope(world);

    {
        pone_val* av = pone_mortalize(world, pone_ary_new(world->universe, 3,
            pone_mortalize(world, pone_int_new(world->universe, 6)),
            pone_mortalize(world, pone_int_new(world->universe, 4)),
            pone_mortalize(world, pone_int_new(world->universe, 3))
        ));

        pone_val* iter = pone_mortalize(world, pone_iter_init(world, av));
        while (true) {
            pone_val* next = pone_mortalize(world, pone_iter_next(world, iter));
            if (next == world->universe->instance_iteration_end) {
                break;
            }
            pone_assign(world, 0, "$_", next);
            pone_builtin_say(world, pone_get_lex(world, "$_"));
        }
    }

    pone_freetmps(world);
    pone_pop_scope(world);

    pone_destroy_world(world);
    pone_universe_destroy(universe);
}

