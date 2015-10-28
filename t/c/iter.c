#include "pone.h"

int main(int argc, char** argv) {
    pone_init();

    pone_universe* universe = pone_universe_init();
    pone_world* world = pone_new_world(universe);

    pone_savetmps(world);
    pone_push_scope(world);

    {
        pone_val* av = pone_mortalize(world, pone_new_ary(world->universe, 3,
            pone_mortalize(world, pone_new_int(world->universe, 6)),
            pone_mortalize(world, pone_new_int(world->universe, 4)),
            pone_mortalize(world, pone_new_int(world->universe, 3))
        ));

        if (setjmp(*(pone_exc_handler_push(world)))) {
            if (pone_type(world->universe->errvar) == PONE_CONTROL_BREAK) {
                fprintf(stderr, "end-of-iter\n");
            } else {
                pone_die(world, world->universe->errvar);
            }
        } else {
            pone_val* iter = pone_mortalize(world, pone_iter_init(world, av));
            while (true) {
                pone_assign(world, 0, "$_", pone_mortalize(world, pone_iter_next(world, iter)));
                pone_builtin_say(world, pone_get_lex(world, "$_"));
            }
        }
    }

    pone_freetmps(world);
    pone_pop_scope(world);

    pone_destroy_world(world);
    pone_universe_destroy(universe);
}

