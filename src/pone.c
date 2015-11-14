#include "pone.h" /* PONE_INC */

#if defined(_WIN32) || defined(_WIN64)
#include <fcntl.h>
#endif

void pone_init(pone_world* world) {
#if defined(_WIN32) || defined(_WIN64)
    setmode(fileno(stdout), O_BINARY);
#endif

    pone_universe* universe = world->universe;

#ifdef TRACE_UNIVERSE
    printf("initializing class mu\n");
#endif
    universe->class_mu = pone_init_mu(world);

#ifdef TRACE_UNIVERSE
    printf("initializing class Class\n");
#endif
    universe->class_class = pone_init_class(world);

    universe->class_any = pone_init_any(world);
    universe->class_cool = pone_init_cool(world);

#ifdef TRACE_UNIVERSE
    printf("initializing class Array\n");
#endif
    pone_ary_init(world);
    assert(universe->class_ary);

    pone_nil_init(world);
    pone_int_init(world);
    pone_str_init(world);
    pone_num_init(world);
    pone_bool_init(world);
    pone_hash_init(world);
    pone_code_init(world);
    pone_range_init(world);
    pone_regex_init(world);
    pone_thread_init(world);
    pone_pair_init(world);
    pone_sock_init(world);
    pone_gc_init(world);

#ifdef TRACE_UNIVERSE
    printf("initializing value IterationEnd\n");
#endif
    universe->instance_iteration_end = pone_obj_new(world, universe->class_mu);

    pone_universe_set_global(universe, "Nil", pone_nil());
    pone_universe_set_global(universe, "IO::Socket::INET", universe->class_io_socket_inet);
    pone_universe_set_global(universe, "Regex", universe->class_regex);

    {
        const char* env = getenv("PONE_GC_LOG");
        if (env && strlen(env) > 0) {
            universe->gc_log = fopen(env, "w");
        }
    }
}
