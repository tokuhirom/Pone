#include "all.h"

int main(int argc, char** argv) {
    pone_world* world = pone_new_world();

    pone_enter(world);

    pone_assign(world, 0, "$x", pone_mortalize(world, pone_new_int(world, 4963)));
    pone_builtin_say(world, pone_get_lex(world, "$x"));
    assert(pone_int_val(pone_get_lex(world, "$x")) == 4963);

    {
        pone_enter(world);
        pone_assign(world, 1, "$x", pone_mortalize(world, pone_new_int(world, 5555)));
        pone_builtin_say(world, pone_get_lex(world, "$x"));
        assert(pone_int_val(pone_get_lex(world, "$x")) == 5555);
        pone_assign(world, 1, "$y", pone_mortalize(world, pone_new_int(world, 1111)));
        pone_leave(world);
    }

    assert(pone_int_val(pone_get_lex(world, "$x")) == 5555);
    assert(pone_int_val(pone_get_lex(world, "$y")) == 1111);

    pone_leave(world);

    pone_destroy_world(world);
}
