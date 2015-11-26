#include "pone.h"
#include "pone_module.h"
#include "pone_val_vec.h"
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
    pone_push_scope(world);
    pone_val* sig_v = pone_int_new(world, sig);
    for (pone_int_t i = 0; i < kv_size(universe->signal_channels[sig]); i++) {
        pone_val* chan = kv_A(universe->signal_channels[sig], i);
        if (!pone_chan_trysend(world, chan, sig_v)) {
            fprintf(stderr, "[pone] cannot send signal to channel(%p): signal:%d\n",
                    chan, sig);
            // this may not critical error.
        }
    }
    pone_pop_scope(world);
}

static void* signal_thread(void* p) {
    pone_world* world = p;
    int sig;
    sigset_t set;
    sigfillset(&set);
#ifdef __APPLE__
    pthread_setname_np("pone signal ^^;");
#else
    pthread_setname_np(pthread_self(), "pone signal ^^;");
#endif
    CHECK_PTHREAD(pthread_mutex_lock(&(world->mutex)));

    THREAD_TRACE("Started signal thread");

    for (;;) {
        if (sigwait(&set, &sig) != 0) {
            perror("sigwait"); // sigwait returns EINVAL.
            abort();
        }

        CHECK_PTHREAD(pthread_mutex_lock(&(world->universe->signal_channels_mutex)));
        handle(world, sig);

        if (world->gc_requested) {
            pone_gc_run(world);
        }
        CHECK_PTHREAD(pthread_mutex_unlock(&(world->universe->signal_channels_mutex)));
    }
    abort(); // should not reach here.
}

// Start signal thread. You should call this before run any threads.
void pone_signal_start_thread(pone_world* world) {
    // mask all signals
    sigset_t set;
    sigfillset(&set);
    CHECK_PTHREAD(pthread_sigmask(SIG_BLOCK, &set, NULL));

    world->universe->signal_world = pone_world_new(world->universe);
    CHECK_PTHREAD(pthread_create(&(world->universe->signal_thread), NULL, signal_thread, world->universe->signal_world));
}

PONE_FUNC(meth_signal_notify) {
    pone_val* chan;
    pone_int_t sig;
    PONE_ARG("signal.notify", "oi", &chan, &sig);

    if (sig >= PONE_SIGNAL_HANDLERS_SIZE) {
        pone_throw_str(world, "Invalid signal code: " PoneIntFmt, sig);
    }

    CHECK_PTHREAD(pthread_mutex_lock(&(world->universe->signal_channels_mutex)));
    // copy channel to signal thread.
    pone_val* copied = pone_val_copy(world->universe->signal_world, chan);
    //  register values to global varaiables.
    kv_push(pone_val*, world->universe->signal_channels[sig], copied);
    CHECK_PTHREAD(pthread_mutex_unlock(&(world->universe->signal_channels_mutex)));

    return pone_nil();
}

PONE_FUNC(meth_signal_kill) {
    pone_int_t pid;
    pone_int_t sig;
    PONE_ARG("signal.kill", "ii", &pid, &sig);

    kill(pid, sig);

    return pone_nil();
}

void pone_signal_init(pone_world* world) {
    pone_val* module = pone_module_new(world, "signal");
    pone_module_put(world, module, "notify", pone_code_new_c(world, meth_signal_notify));
    pone_module_put(world, module, "kill", pone_code_new_c(world, meth_signal_kill));

#define SETSIG(sig) \
    pone_module_put(world, module, #sig, pone_int_new(world, sig))

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
#ifdef SIGPOLL
    SETSIG(SIGPOLL);
#endif
    SETSIG(SIGPROF);
    SETSIG(SIGSYS);
    SETSIG(SIGTRAP);
    SETSIG(SIGURG);
    SETSIG(SIGVTALRM);
    SETSIG(SIGXCPU);
    SETSIG(SIGXFSZ);

#undef SETSIG

    pone_universe_set_global(world->universe, "signal", module);
}
