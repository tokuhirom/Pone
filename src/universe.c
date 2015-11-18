#include "pone.h" /* PONE_INC */
#include "oniguruma.h"
#include <errno.h>
#include "utlist.h"

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

    CHECK_PTHREAD(pthread_mutex_init(&(universe->universe_mutex), NULL));
    CHECK_PTHREAD(pthread_cond_init(&(universe->thread_terminate_cond), NULL));

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

pone_int_t pone_count_alive_threads(pone_universe* universe) {
    pone_world* world;
    pone_int_t r = 0;
    CDL_FOREACH(universe->world_head, world) {
        if (world->code) {
            r++;
        }
        world = world->next;
    }
    return r;
}

// wait until threads finished jobs.
void pone_universe_wait_threads(pone_universe* universe) {
    UNIVERSE_LOCK(universe);
    while (true) {
      pone_int_t n = pone_count_alive_threads(universe);
      if (n == 0) {
          break;
      }
      WORLD_TRACE("There's %ld worlds", n);
      CHECK_PTHREAD(pthread_cond_wait(&(universe->thread_terminate_cond), &(universe->universe_mutex)));
    }
    UNIVERSE_UNLOCK(universe);
}

void pone_universe_destroy(pone_universe* universe) {
    pone_universe_wait_threads(universe);

    CHECK_PTHREAD(pthread_cond_destroy(&(universe->thread_terminate_cond)));
    CHECK_PTHREAD(pthread_mutex_destroy(&(universe->universe_mutex)));

    kh_destroy(str, universe->globals);

    free(universe);
}

// gc mark
void pone_universe_mark(pone_universe* universe) {
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

