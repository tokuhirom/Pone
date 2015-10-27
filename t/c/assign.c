#include "pone_all.h"

int main(int argc, char** argv) {
    pone_universe* universe = pone_universe_init();
    pone_world* world = pone_new_world(universe);

    pone_savetmps(world);
    pone_push_scope(world);

    pone_assign(world, 0, "$x", pone_mortalize(world, pone_new_int(world, 4963)));
    pone_builtin_say(world, pone_get_lex(world, "$x"));
    assert(pone_int_val(pone_get_lex(world, "$x")) == 4963);

    {
        pone_savetmps(world);
        pone_push_scope(world);

        pone_assign(world, 1, "$x", pone_mortalize(world, pone_new_int(world, 5555)));
        pone_builtin_say(world, pone_get_lex(world, "$x"));
        assert(pone_int_val(pone_get_lex(world, "$x")) == 5555);
        pone_assign(world, 1, "$y", pone_mortalize(world, pone_new_int(world, 1111)));
        pone_freetmps(world);
        pone_pop_scope(world);
    }

    assert(pone_int_val(pone_get_lex(world, "$x")) == 5555);
    assert(pone_int_val(pone_get_lex(world, "$y")) == 1111);

    pone_freetmps(world);
    pone_pop_scope(world);

    pone_destroy_world(world);
    pone_universe_destroy(universe);
}
