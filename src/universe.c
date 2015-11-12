#include "pone.h" /* PONE_INC */
#include "rockre.h"

#ifdef __GLIBC__
#include <execinfo.h>
#endif

void pone_universe_default_err_handler(pone_world* world) {
    assert(world->errvar);
    pone_val* str = pone_stringify(world, world->errvar);
    fwrite("\n!!!!!!!!! ( Д ) ..._。..._。 !!!!!!!!!\n\n", 1, strlen("\n!!!!!!!!! ( Д ) ..._。..._。 !!!!!!!!!\n\n"), stderr);
    fwrite(pone_str_ptr(str), 1, pone_str_len(str), stderr);
    fwrite("\n\n", 1, strlen("\n\n"), stderr);

#ifdef __GLIBC__
    {
        void *trace[128];
        int n = backtrace(trace, sizeof(trace) / sizeof(trace[0]));
        backtrace_symbols_fd(trace, n, 1);
    }
#endif

    exit(1);
}

pone_universe* pone_universe_init() {
    pone_universe*universe = malloc(sizeof(pone_universe));
    if (!universe) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    memset(universe, 0, sizeof(pone_universe));
    universe->arena_last = universe->arena_head = malloc(sizeof(pone_arena));
    if (!universe->arena_last) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    memset(universe->arena_last, 0, sizeof(pone_arena));

    universe->rockre = rockre_new();

    universe->globals = kh_init(str);

#ifdef TRACE_UNIVERSE
    printf("initializing class mu\n");
#endif
    universe->class_mu = pone_init_mu(universe);

#ifdef TRACE_UNIVERSE
    printf("initializing class Class\n");
#endif
    universe->class_class = pone_init_class(universe);

    universe->class_any = pone_init_any(universe);
    universe->class_cool = pone_init_cool(universe);

#ifdef TRACE_UNIVERSE
    printf("initializing class Array\n");
#endif
    pone_ary_init(universe);
    assert(universe->class_ary);

    pone_nil_init(universe);
    pone_int_init(universe);
    pone_str_init(universe);
    pone_num_init(universe);
    pone_bool_init(universe);
    pone_hash_init(universe);
    pone_code_init(universe);
    assert(universe->class_range == NULL);
    pone_range_init(universe);
    pone_regex_init(universe);
    assert(universe->class_io_socket_inet == NULL);
    pone_thread_init(universe);
    pone_pair_init(universe);
    pone_sock_init(universe);
    pone_gc_init(universe);

#ifdef TRACE_UNIVERSE
    printf("initializing value IterationEnd\n");
#endif
    universe->instance_iteration_end = pone_obj_new(universe, universe->class_mu);

    pone_universe_set_global(universe, "Nil", pone_nil());
    pone_universe_set_global(universe, "IO::Socket::INET", universe->class_io_socket_inet);
    pone_universe_set_global(universe, "Regex", universe->class_regex);

#undef PUT

    {
        const char* env = getenv("PONE_GC_LOG");
        if (env && strlen(env) > 0) {
            universe->gc_log = fopen(env, "w");
        }
    }

    return universe;
}

void pone_universe_set_global(pone_universe* universe, const char* key, pone_val* val) {
    int ret;
    khint_t k = kh_put(str, universe->globals, key, &ret);
    if (ret == -1) {
        abort();
    }
    kh_val(universe->globals, k) = val;
}

void pone_universe_destroy(pone_universe* universe) {
    while (universe->thread_num>0) {
        (void)pone_thread_join(universe, universe->threads->thread);
    }

    kh_destroy(str, universe->globals);

    rockre_destroy(universe->rockre);

    pone_arena* a = universe->arena_head;
    while (a) {
        pone_arena* next = a->next;
        free(a);
        a = next;
    }
    free(universe);
}

// gc mark
void pone_universe_mark(pone_universe* universe) {
    {
        const char* k;
        pone_val* v;
        kh_foreach(universe->globals, k, v, {
            pone_gc_mark_value(v);
        });
    }

    pone_gc_mark_value(universe->instance_iteration_end);

    pone_gc_mark_value(universe->class_io_socket_inet);
    pone_gc_mark_value(universe->class_pair);
    pone_gc_mark_value(universe->class_thread);
    pone_gc_mark_value(universe->class_match);
    pone_gc_mark_value(universe->class_regex);
    pone_gc_mark_value(universe->class_range);
    pone_gc_mark_value(universe->class_code);
    pone_gc_mark_value(universe->class_hash);
    pone_gc_mark_value(universe->class_bool);
    pone_gc_mark_value(universe->class_num);
    pone_gc_mark_value(universe->class_int);
    pone_gc_mark_value(universe->class_str);
    pone_gc_mark_value(universe->class_nil);
    pone_gc_mark_value(universe->class_ary);
    pone_gc_mark_value(universe->class_any);
    pone_gc_mark_value(universe->class_cool);
    pone_gc_mark_value(universe->class_class);
    pone_gc_mark_value(universe->class_mu);

    pone_world* world = universe->world_head;
    while (world) {
        pone_world_mark(world);
        assert(world != world->next);
        world = world->next;
    }
}

void pone_gc_log(pone_universe* universe, const char* fmt, ...) {
    if (universe->gc_log) {
        va_list args;
        va_start(args, fmt);
        vfprintf(universe->gc_log, fmt, args);
        va_end(args);
    }
}

