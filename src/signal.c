#include "pone.h"
#include "kvec.h"

#include <signal.h>

static inline void handle(pone_world* world, int sig) {
    pone_universe* universe = world->universe;
    if (kv_size(universe->signal_channels[sig]) == 0) {
        // There's no signal handlers.

        // Then, remove sigmask
        sigset_t set;
        sigemptyset(&set);
        sigaddset(&set, sig);
        CHECK_PTHREAD(pthread_sigmask(SIG_UNBLOCK, &set, NULL));

        signal(sig, SIG_DFL);

        // Send signal to me.
        CHECK_PTHREAD(pthread_kill(pthread_self(), sig));

        // restore sigmask
        sigfillset(&set);
        CHECK_PTHREAD(pthread_sigmask(SIG_BLOCK, &set, NULL));
    }

    UNIVERSE_LOCK(universe);
    pone_push_scope(world);
    pone_val* sig_v = pone_int_new(world, sig);
    for (pone_int_t i=0; i<kv_size(universe->signal_channels[sig]); i++) {
        pone_val* chan = kv_A(universe->signal_channels[sig], i);
        if (!pone_chan_trysend(world, chan, sig_v)) {
            fprintf(stderr, "[pone] cannot send signal to channel(%p): signal:%d\n",
                    chan, sig);
            // this may not critical error.
        }
    }
    pone_pop_scope(world);
    UNIVERSE_UNLOCK(universe);
}

static void* signal_thread(void* p) {
    pone_universe* universe = p;
    int sig;
    sigset_t set;
    sigfillset(&set);
    pone_world* world = pone_world_new(universe);
    CHECK_PTHREAD(pthread_mutex_lock(&(world->mutex)));

    for (;;) {
        if (sigwait(&set, &sig) != 0) {
            perror("sigwait"); // sigwait returns EINVAL.
            abort();
        }
        handle(world, sig);

        if (world->gc_requested) {
            pone_gc_run(world);
        }
    }
    abort(); // should not reach here.
}

// Start signal thread. You should call this before run any threads.
void pone_signal_start_thread(pone_world* world) {
    // mask all signals
    sigset_t set;
    sigfillset(&set);
    CHECK_PTHREAD(pthread_sigmask(SIG_BLOCK, &set, NULL));

    pthread_t thread;
    CHECK_PTHREAD(pthread_create(&thread, NULL, signal_thread, world->universe));
    pthread_setname_np(thread, "pone signal ^^;");
}

static pone_val* meth_signal_notify(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 2);

    pone_val* chan   = va_arg(args, pone_val*);
    pone_int_t sig = pone_intify(world, va_arg(args, pone_val*));

    if (sig >= PONE_SIGNAL_HANDLERS_SIZE) {
        pone_throw_str(world, "Invalid signal code: " PoneIntFmt, sig);
    }

    UNIVERSE_LOCK(world->universe);
    kv_push(pone_val*, world->universe->signal_channels[sig], chan);
    UNIVERSE_UNLOCK(world->universe);

    return pone_nil();
}

#define DEFINE_CONST_INT(name, val) \
    static pone_val* meth_signal_sigint(pone_world* world, pone_val* self, int n, va_list args) { \
        assert(n == 0); \
        return pone_int_new(world, SIGINT); \
    }

static pone_val* meth_signal_sigint(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);
    return pone_int_new(world, SIGINT);
}

static pone_val* meth_signal_sigterm(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);
    return pone_int_new(world, SIGTERM);
}

void pone_signal_init(pone_world* world) {
    pone_universe* universe = world->universe;

    pone_val* klass = pone_class_new(world, "Signal", strlen("Signal"));
    pone_class_push_parent(world, klass, universe->class_mu);
    pone_add_method_c(world, universe->class_thread, "notify", strlen("notify"), meth_signal_notify);
    pone_add_method_c(world, universe->class_thread, "SIGINT", strlen("SIGINT"), meth_signal_sigint);
    pone_add_method_c(world, universe->class_thread, "SIGTERM", strlen("SIGTERM"), meth_signal_sigint);
    pone_class_compose(world, universe->class_thread);
    pone_universe_set_global(universe, "Signal", universe->class_thread);
}

