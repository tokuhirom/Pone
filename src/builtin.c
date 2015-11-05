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
        pone_throw_str(world, "you can't call abs() for non-numeric value");
        abort();
    }
}

pone_val* pone_builtin_print(pone_world* world, pone_val* val) {
    pone_val* str = pone_stringify(world, val);
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
    pone_val* str = pone_mortalize(world, pone_stringify(world, key));
    const char* len = getenv(pone_str_ptr(str));
    if (len) {
        return pone_mortalize(world, pone_str_new(world->universe, len, strlen(len)));
    } else {
        return pone_nil();
    }
}

pone_val* pone_builtin_sleep(pone_world* world, pone_val* vi) {
    // TODO Time::HiRes
    int i = pone_intify(world, vi);
    sleep(i);
    return pone_nil();
}

int pone_signal_received;

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

pone_val* pone_builtin_signal(pone_world* world, pone_val* sig_val, pone_val* code) {
    int sig = pone_intify(world, sig_val);

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
            pone_throw_str(world, "cannot set signal");
        }
#else
        pone_throw_str(world, "not implemented on windows");
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
    pone_throw(world, msg);
    return pone_nil();
}

pone_val* pone_builtin_printf(pone_world* world, pone_val* fmt, ...) {
    fmt = pone_stringify(world, fmt);

#define PRINTF_BUFSIZ 256
    char fmt_buf[PRINTF_BUFSIZ];
    char dst_buf[PRINTF_BUFSIZ];

    va_list args;
    va_start(args, fmt);

    const char* p = pone_str_ptr(fmt);
    const char* end = p+pone_str_len(fmt);
    while (p < end) {
        switch (*p) {
        case '%': {
            const char* fmt_p = p;
            bool done = false;
            ++p;
            while ((!done) && p < end) {
                switch (*p) {
                case 's': {
                    // TODO %-10s
                    pone_val* v = va_arg(args, pone_val*);
                    pone_val* str = pone_stringify(world, v);
                    fwrite(pone_str_ptr(str), sizeof(char), pone_str_len(str), stdout);
                    pone_refcnt_dec(world->universe, str);
                    done = true;
                    break;
                }
                case 'd':
                case 'f': {
                    char fmt = *p;
                    ++p;
                    pone_val* v = va_arg(args, pone_val*);
                    if (p - fmt_p > PRINTF_BUFSIZ-1) {
                        pone_throw_str(world, "[printf] format string too long");
                    }

                    memcpy(fmt_buf, fmt_p, p-fmt_p);
                    fmt_buf[p-fmt_p] = '\0';

                    int printed;
                    switch (fmt) {
                        case 'f': {
                            pone_num_t n = pone_numify(world, v);
                            printed = snprintf(dst_buf, PRINTF_BUFSIZ-1, fmt_buf, n);
                            break;
                        }
                        case 'd': {
                            int i = pone_intify(world, v);
                            printed = snprintf(dst_buf, PRINTF_BUFSIZ-1, fmt_buf, i);
                            break;
                        }
                        default:
                            abort();
                    }
                    if (printed > PRINTF_BUFSIZ-1) {
                        pone_throw_str(world, "[printf] printf buffer overrun");
                    }
#undef PRINTF_BUFSIZ
                    fwrite(dst_buf, sizeof(char), printed, stdout);
                    done = true;
                    break;
                }
                case '%': {
                    putc('%', stdout);
                    done = true;
                    break;
                }
                default:
                    ++p;
                    break;
                }
            }
            if (done) {
                break;
            }
            pone_throw_str(world, "invalid format for printf");
        }
        default:
            putc(*p, stdout);
            ++p;
            break;
        }
    }

    va_end(args);
    return pone_nil();
}

