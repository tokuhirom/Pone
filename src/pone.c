#include "pone.h" /* PONE_INC */
#include "oniguruma.h"
#include <math.h>

#if defined(_WIN32) || defined(_WIN64)
#include <fcntl.h>
#endif

void pone_os_init(pone_world* world);
void pone_file_init(pone_world* world);
void pone_module_init(pone_world* world);
void pone_time_init(pone_world* world);
void pone_tmpfile_init(pone_world* world);

void pone_init(pone_universe* universe) {
#if defined(_WIN32) || defined(_WIN64)
    setmode(fileno(stdout), O_BINARY);
#endif

    onig_init();

    // create new world to initialize built-in classes.
    pone_world* world = pone_world_new(universe);

    // Do not reuse this world.
    CHECK_PTHREAD(pthread_mutex_lock(&(world->mutex)));

#ifdef TRACE_UNIVERSE
    printf("initializing class mu\n");
#endif

#ifdef TRACE_UNIVERSE
    printf("initializing class Class\n");
#endif
    universe->class_class = pone_init_class(world);

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
    pone_gc_init(world);
    pone_channel_init(world);
    pone_opaque_init(world);
    pone_errno_init(world);
    pone_runtime_init(world);
    pone_file_init(world);
    pone_module_init(world);

    pone_signal_init(world);
    pone_os_init(world);
    pone_builtin_init(world);
    pone_time_init(world);
    pone_tmpfile_init(world);

    // init $*INC
    universe->inc = pone_ary_new(world, 1, pone_str_new_const(world, "blib", strlen("blib")));

    // TODO
    // pone_universe_set_global(universe, "pi", pone_num_new(world, M_PI));

    {
        pone_val* klass = pone_class_new(world, "IterationEnd", strlen("IterationEnd"));
        universe->instance_iteration_end = pone_obj_new(world, klass);
    }

    {
        const char* env = getenv("PONE_GC_LOG");
        if (env && strlen(env) > 0) {
            universe->gc_log = fopen(env, "w");
            if (!universe->gc_log) {
                fprintf(stderr, "Cannot open %s\n", env);
            }
        }
    }

    // start signal thread
    pone_signal_start_thread(world);
}
