#include "pone_all.h"

int main(int argc, char** argv) {
    pone_world* world = pone_new_world();

    pone_push_scope(world);
    assert(world->lex->refcnt == 1);
    pone_pop_scope(world);

    pone_destroy_world(world);
}
