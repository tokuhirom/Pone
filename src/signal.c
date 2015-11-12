#include "pone.h"

#include <signal.h>

static int pone_signal_received;

void pone_signal_handle(pone_world* world) {
    if (pone_signal_received > 0) {
        int sig = pone_signal_received;
        pone_signal_received = 0;
        pone_val* code = world->universe->signal_handlers[sig];
        pone_code_call(world, code, pone_nil(), 0);
    }
}

static void sig_handler(int sig) {
    pone_signal_received = sig;
}

// send internal signal like PONE_SIG_GC
void pone_send_private_sig(int sig) {
    pone_signal_received = sig;
}

void pone_signal_register_handler(pone_world* world, pone_int_t sig, pone_val* code) {
    if (pone_defined(code)) {
#ifndef _WIN32
        struct sigaction act = {
            .sa_handler = sig_handler,
            .sa_flags = 0,
        };
        sigemptyset(&act.sa_mask);

        if (sigaction(sig, &act, NULL) == 0) {
            world->universe->signal_handlers[sig] = code;
        } else {
            pone_throw_str(world, "cannot set signal");
        }
#else
        pone_throw_str(world, "not implemented on windows");
#endif
    } else {
        signal(sig, SIG_DFL);
    }
}

