#include "pone.h" /* PONE_INC */
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

pone_val*  pone_builtin_dd(pone_world* world, pone_val* val) {
    pone_dd(world->universe, val);
    return pone_nil();
}

pone_val*  pone_builtin_abs(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_INT: {
        int i = pone_int_val(val);
        if (i < 0) {
            return pone_mortalize(world, pone_int_new(world->universe, -i));
        } else {
            return val;
        }
    }
    case PONE_NUM: {
        // TODO: NV
    }
    default:
        pone_die_str(world, "you can't call abs() for non-numeric value");
        abort();
    }
}

pone_val* pone_builtin_print(pone_world* world, pone_val* val) {
    pone_val* str = pone_to_str(world->universe, val);
    fwrite(pone_str_ptr(str), sizeof(char), pone_str_len(str), stdout);
    pone_refcnt_dec(world->universe, str);
    return pone_nil();
}

pone_val* pone_builtin_say(pone_world* world, pone_val* val) {
    pone_builtin_print(world, val);
    fwrite("\n", sizeof(char), 1, stdout);
    return pone_nil();
}

pone_val* pone_builtin_elems(pone_world* world, pone_val* val) {
    return pone_mortalize(world, pone_int_new(world->universe, pone_elems(world, val)));
}

pone_val* pone_builtin_time(pone_world* world) {
    return pone_mortalize(world, pone_int_new(world->universe, time(NULL)));
}

pone_val* pone_builtin_getenv(pone_world* world, pone_val* key) {
    pone_val* str = pone_mortalize(world, pone_to_str(world->universe, key));
    const char* len = getenv(pone_str_ptr(str));
    if (len) {
        return pone_mortalize(world, pone_str_new(world->universe, len, strlen(len)));
    } else {
        return pone_nil();
    }
}

pone_val* pone_builtin_sleep(pone_world* world, pone_val* vi) {
    // TODO Time::HiRes
    int i = pone_to_int(world, vi);
    sleep(i);
    return pone_nil();
}

int pone_signal_received;

void pone_signal_handle(pone_world* world) {
    if (pone_signal_received > 0) {
        int sig = pone_signal_received;
        pone_signal_received = 0;
        pone_val* code = world->universe->signal_handlers[sig];
        pone_code_call(world, code, 0);
    }
}

static void sig_handler(int sig) {
    pone_signal_received = sig;
}

pone_val* pone_builtin_signal(pone_world* world, pone_val* sig_val, pone_val* code) {
    int sig = pone_to_int(world, sig_val);

    if (pone_defined(code)) {
#ifndef _WIN32
        struct sigaction act = {
            .sa_handler = sig_handler,
            .sa_flags = 0,
        };
        sigemptyset(&act.sa_mask);

        if (sigaction(sig, &act, NULL) == 0) {
            printf("Set sig %d\n", sig);
            pone_refcnt_inc(world->universe, code);
            world->universe->signal_handlers[sig] = code;
        } else {
            pone_die_str(world, "cannot set signal");
        }
#else
        pone_die_str(world, "not implemented on windows");
#endif
    } else {
        if (world->universe->signal_handlers[sig]) {
            pone_refcnt_dec(world->universe, world->universe->signal_handlers[sig]);
        }
        signal(sig, SIG_DFL);
    }

    return pone_nil();
}

pone_val* pone_builtin_die(pone_world* world, pone_val* msg) {
    pone_die(world, msg);
    return pone_nil();
}

