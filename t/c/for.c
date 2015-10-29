#include "pone.h"

// for [1,2,3] { say $_ }

int main(int argc, const char** argv) {
  pone_universe* universe;
  pone_world* world;
  pone_init();
  universe = pone_universe_init();
  if (setjmp(universe->err_handlers[0])) {
    pone_universe_default_err_handler(universe);
  }
  world = pone_world_new(universe);
  pone_savetmps(world);
  pone_push_scope(world);
  pone_val* iter = pone_mortalize(
      world,
      pone_iter_init(
          world,
          pone_mortalize(
              world,
              pone_ary_new(
                  world->universe, 3,
                  pone_mortalize(world, pone_int_new(world->universe, 1)),
                  pone_mortalize(world, pone_int_new(world->universe, 2)),
                  pone_mortalize(world, pone_int_new(world->universe, 3))))));
  while (true) {
    pone_val* next = pone_mortalize(world, pone_iter_next(world, iter));
    if (next == world->universe->instance_iteration_end) {
      break;
    }
    pone_assign(world, 0, "$_", pone_iter_next(world, iter));
    pone_builtin_say(world, pone_get_lex(world, "$_"));
    pone_signal_handle(world);
  }
  pone_signal_handle(world);

  pone_freetmps(world);
  pone_pop_scope(world);
  pone_destroy_world(world);
  pone_universe_destroy(universe);
}
