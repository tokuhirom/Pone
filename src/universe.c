#include "pone.h" /* PONE_INC */
#include "oniguruma.h"
#include <errno.h>
#include "kvec.h"

#ifdef __GLIBC__
#include <execinfo.h>
#endif

void pone_universe_default_err_handler(pone_world* world) {
    assert(world->errvar);
    pone_val* str = pone_stringify(world, world->errvar);
    fwrite("\n!!!!!!!!! ( Д ) ..._。..._。 !!!!!!!!!\n\n", 1, strlen("\n!!!!!!!!! ( Д ) ..._。..._。 !!!!!!!!!\n\n"), stderr);
    fwrite(pone_str_ptr(str), 1, pone_str_len(str), stderr);
    fwrite("\n\n", 1, strlen("\n\n"), stderr);

    if (world->stacktrace) {
        for (pone_int_t i=0; i<pone_ary_elems(world->stacktrace); ++i) {
            printf("%5ld: %s\n", pone_ary_elems(world->stacktrace)-i,
                    pone_str_ptr(pone_str_c_str(world, pone_ary_at_pos(world->stacktrace, i))));
        }
    }

    exit(1);
}

pone_universe* pone_universe_init() {
    pone_universe*universe = malloc(sizeof(pone_universe));
    if (!universe) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    memset(universe, 0, sizeof(pone_universe));

    CHECK_PTHREAD(pthread_mutex_init(&(universe->signal_channels_mutex), NULL));
    CHECK_PTHREAD(pthread_mutex_init(&(universe->worker_worlds_mutex), NULL));
    CHECK_PTHREAD(pthread_cond_init(&(universe->worker_fin_cond), NULL));

    // initialize signal channels.
    for (int i=0; i<PONE_SIGNAL_HANDLERS_SIZE; ++i) {
        kv_init(universe->signal_channels[i]);
    }

    universe->globals = kh_init(str);

    return universe;
}

// Do not call this from C extensions. C extension should use EXPORT instead.
void pone_universe_set_global(pone_universe* universe, const char* key, pone_val* val) {
    int ret;
    khint_t k = kh_put(str, universe->globals, key, &ret);
    if (ret == -1) {
        abort();
    }
    kh_val(universe->globals, k) = val;
}

pone_int_t pone_count_alive_threads(pone_universe* universe) {
    pone_world* world = universe->worker_worlds;
    pone_int_t r = 0;
    pone_int_t d = 0;
    while (world) {
        if (world->code) {
            WORLD_TRACE("ALIVE THREAD: %lx", world->thread_id);
            r++;
        } else {
            WORLD_TRACE("DEAD THREAD: %lx", world->thread_id);
            d++;
        }
        world = world->next;
    }
    WORLD_TRACE("ALIVE THREAD: %ld DEAD THREAD: %ld", r, d);
    return r;
}

// wait until threads finished jobs.
void pone_universe_wait_threads(pone_universe* universe) {
    CHECK_PTHREAD(pthread_mutex_lock(&(universe->worker_worlds_mutex)));
    while (true) {
        if (universe->worker_worlds) {
            pone_int_t n = pone_count_alive_threads(universe);
            if (n == 0) {
                break;
            }
        }
        CHECK_PTHREAD(pthread_cond_wait(&(universe->worker_fin_cond), &(universe->worker_worlds_mutex)));
    }
    CHECK_PTHREAD(pthread_mutex_unlock(&(universe->worker_worlds_mutex)));
}

void pone_universe_destroy(pone_universe* universe) {
    CHECK_PTHREAD(pthread_cancel(universe->signal_thread));
    void* retval;
    CHECK_PTHREAD(pthread_join(universe->signal_thread, &retval));
    CHECK_PTHREAD(pthread_cond_destroy(&(universe->worker_fin_cond)));
    CHECK_PTHREAD(pthread_mutex_destroy(&(universe->signal_channels_mutex)));
    CHECK_PTHREAD(pthread_mutex_destroy(&(universe->worker_worlds_mutex)));

    kh_destroy(str, universe->globals);

    free(universe);
}

// gc mark
void pone_universe_mark(pone_universe* universe) {
//  pone_world* world = universe->worker_worlds;
//  while (world) {
//      pone_world_mark(world);
//      assert(world != world->next);
//      world = world->next;
//  }

    for (int i=0; i<PONE_SIGNAL_HANDLERS_SIZE; ++i) {
        for (int j=0; j<kv_size(universe->signal_channels[i]); j++) {
            pone_gc_mark_value(kv_A(universe->signal_channels[i], j));
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

