#include "pone.h"

pone_val* pone_user_func_y(pone_world* world, int n, va_list args) {
    pone_savetmps(world);
    pone_push_scope(world);

    assert(n==1);

    pone_assign(world, 0, "$n", va_arg(args, pone_val*));

    return pone_add(world,
            pone_get_lex(world, "$x"),
            pone_get_lex(world, "$n"));
}

int main(int argc, char** argv) {
    pone_universe* universe = pone_universe_init();
    pone_world* world = pone_world_new(universe);

    pone_savetmps(world);
    pone_push_scope(world);

    pone_assign(world, 0, "$x", pone_mortalize(world, pone_int_new(world->universe, 2)));

    pone_assign(world, 0, "&y", pone_mortalize(world, pone_code_new(world, pone_user_func_y)));

    {
        pone_savetmps(world);
        pone_push_scope(world);

        pone_assign(world, 0, "$x", pone_mortalize(world, pone_int_new(world->universe, 9)));

    pone_val* got = pone_code_call(world, pone_get_lex(world, "&y"), 1, pone_mortalize(world, pone_int_new(world->universe, 100)));
        pone_builtin_say(world, got);

        pone_freetmps(world);
        pone_pop_scope(world);
    }

    pone_freetmps(world);
    pone_pop_scope(world);

    pone_destroy_world(world);
    pone_universe_destroy(universe);
}

