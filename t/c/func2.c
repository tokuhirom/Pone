#include "pone.h"

pone_val* pone_user_func_y(pone_world* world, int n, va_list args) {
    assert(n==0);

    return pone_mortalize(world, pone_new_int(world->universe, 1)); // we must not mortalize return value
}

int main(int argc, char** argv) {
    pone_universe* universe = pone_universe_init();
    pone_world* world = pone_new_world(universe);

    pone_savetmps(world);
    pone_push_scope(world);

    pone_assign(world, 0, "&y", pone_mortalize(world, pone_code_new(world, pone_user_func_y)));

    pone_builtin_say(world, pone_code_call(world, pone_get_lex(world, "&y"), 0));

    pone_pop_scope(world);
    pone_freetmps(world);

    assert(world->tmpstack_idx == 0);

    pone_destroy_world(world);
    pone_universe_destroy(universe);
}

