#include "../../lib/Pone/runtime.c"

int main(int argc, char** argv) {
    pone_world* world = pone_new_world();

    pone_enter(world);

    pone_assign(world, "$x", pone_mortalize(world, pone_new_int(world, 4963)));
    pone_builtin_say(world, pone_get_lex(world, "$x"));

    pone_leave(world);

    pone_destroy_world(world);
}
