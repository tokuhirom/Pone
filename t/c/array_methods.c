#include "pone.h"


// --------------- vvvv main function vvvv -------------------
int main(int argc, const char **argv) {
    pone_universe* universe;

    pone_init();
    universe = pone_universe_init();
    if (setjmp(universe->err_handlers[0])) {
        pone_universe_default_err_handler(universe);
    }
    pone_world* world = pone_world_new(universe);
    pone_savetmps(world);
    pone_push_scope(world);

    // my $ary = [5963];
    pone_val* ary = pone_mortalize(world, pone_ary_new(world->universe, 1,
        pone_mortalize(world, pone_int_new(world->universe, 5963))
    ));

    // say($ary.elems);
    pone_builtin_say(world, pone_call_method(world, ary, "elems", 0));
    pone_signal_handle(world);

    // $ary.append(4649);
    pone_val* v5963 = pone_mortalize(world, pone_int_new(world->universe, 5963));
    pone_call_method(world,
            ary,
            "append", 1,
            v5963
            );

    pone_builtin_say(world, pone_call_method(world, ary, "elems", 0));

    for (int i=0; i<100; ++i) {
        // $ary.append(4649);
        pone_call_method(world,
                ary,
                "append",
                1,
                pone_int_new(world->universe, 3)
                );
    }

    pone_builtin_say(world, pone_call_method(world, ary, "elems", 0));

    pone_signal_handle(world);

    pone_freetmps(world);
    pone_pop_scope(world);
    pone_destroy_world(world);
    pone_universe_destroy(universe);
}

