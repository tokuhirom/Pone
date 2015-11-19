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

    // Send signal to channels.
    CHECK_PTHREAD(pthread_mutex_lock(&(universe->signal_channels_mutex)));
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
    CHECK_PTHREAD(pthread_mutex_unlock(&(universe->signal_channels_mutex)));
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

PONE_FUNC(meth_signal_notify) {
    pone_val* chan;
    pone_int_t sig;
    PONE_ARG("Signal#notify", "oi", &chan, &sig);

    if (sig >= PONE_SIGNAL_HANDLERS_SIZE) {
        pone_throw_str(world, "Invalid signal code: " PoneIntFmt, sig);
    }

    CHECK_PTHREAD(pthread_mutex_lock(&(world->universe->signal_channels_mutex)));
    kv_push(pone_val*, world->universe->signal_channels[sig], chan);
    CHECK_PTHREAD(pthread_mutex_unlock(&(world->universe->signal_channels_mutex)));

    return pone_nil();
}

PONE_FUNC(meth_signal_kill) {
    pone_int_t pid;
    pone_int_t sig;
    PONE_ARG("Signal#kill", "ii", &pid, &sig);

    kill(pid, sig);

    return pone_nil();
}

#define DEFSIG(sig) \
    PONE_FUNC(meth_signal_##sig) { \
        PONE_ARG("Signal#" #sig, ""); \
        return pone_int_new(world, sig); \
    }

DEFSIG(SIGHUP);
DEFSIG(SIGINT);
DEFSIG(SIGQUIT);
DEFSIG(SIGILL);
DEFSIG(SIGABRT);
DEFSIG(SIGFPE);
DEFSIG(SIGKILL);
DEFSIG(SIGSEGV);
DEFSIG(SIGPIPE);
DEFSIG(SIGALRM);
DEFSIG(SIGTERM);
DEFSIG(SIGUSR1);
DEFSIG(SIGUSR2);
DEFSIG(SIGCHLD);
DEFSIG(SIGCONT);
DEFSIG(SIGSTOP);
DEFSIG(SIGTSTP);
DEFSIG(SIGTTIN);
DEFSIG(SIGTTOU);
DEFSIG(SIGBUS);
DEFSIG(SIGPOLL);
DEFSIG(SIGPROF);
DEFSIG(SIGSYS);
DEFSIG(SIGTRAP);
DEFSIG(SIGURG);
DEFSIG(SIGVTALRM);
DEFSIG(SIGXCPU);
DEFSIG(SIGXFSZ);

#undef DEFSIG

void pone_signal_init(pone_world* world) {
    pone_universe* universe = world->universe;

    pone_val* klass = pone_class_new(world, "Signal", strlen("Signal"));
    pone_class_push_parent(world, klass, universe->class_mu);
    PONE_REG_METHOD(universe->class_thread, "notify", meth_signal_notify);
    PONE_REG_METHOD(universe->class_thread, "kill", meth_signal_kill);
#define SETSIG(sig) \
    pone_obj_set_ivar(world, universe->class_thread, #sig, pone_int_new(world, sig));

SETSIG(SIGHUP);
SETSIG(SIGINT);
SETSIG(SIGQUIT);
SETSIG(SIGILL);
SETSIG(SIGABRT);
SETSIG(SIGFPE);
SETSIG(SIGKILL);
SETSIG(SIGSEGV);
SETSIG(SIGPIPE);
SETSIG(SIGALRM);
SETSIG(SIGTERM);
SETSIG(SIGUSR1);
SETSIG(SIGUSR2);
SETSIG(SIGCHLD);
SETSIG(SIGCONT);
SETSIG(SIGSTOP);
SETSIG(SIGTSTP);
SETSIG(SIGTTIN);
SETSIG(SIGTTOU);
SETSIG(SIGBUS);
SETSIG(SIGPOLL);
SETSIG(SIGPROF);
SETSIG(SIGSYS);
SETSIG(SIGTRAP);
SETSIG(SIGURG);
SETSIG(SIGVTALRM);
SETSIG(SIGXCPU);
SETSIG(SIGXFSZ);

#undef SETSIG
    pone_class_compose(world, universe->class_thread);
    pone_universe_set_global(universe, "Signal", universe->class_thread);
}

