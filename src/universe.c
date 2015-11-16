#include "pone.h" /* PONE_INC */
#include "rockre.h"
#include <errno.h>

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

    int r=0;
    if ((r=pthread_mutex_init(&(universe->gc_mutex), NULL))!=0) {
        errno=r;
        perror("pthread_mutex_init");
        abort();
    }
    if ((r=pthread_mutex_init(&(universe->universe_mutex), NULL))!=0) {
        errno=r;
        perror("pthread_mutex_init");
        abort();
    }
    if ((r=pthread_cond_init(&(universe->gc_cond), NULL))!=0) {
        errno=r;
        perror("pthread_cond_init");
        abort();
    }

    universe->rockre = rockre_new();

    universe->globals = kh_init(str);


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

    pthread_mutex_destroy(&(universe->gc_mutex));
    pthread_cond_destroy(&(universe->gc_cond));
    pthread_mutex_destroy(&(universe->universe_mutex));

    kh_destroy(str, universe->globals);

    rockre_destroy(universe->rockre);

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


    for (int i=0; i<PONE_SIGNAL_HANDLERS_SIZE; ++i) {
        pone_val* handler = universe->signal_handlers[i];
        if (handler) {
            pone_gc_mark_value(handler);
        }
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

