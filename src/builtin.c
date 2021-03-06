#include "pone.h" /* PONE_INC */
#include "pone_exc.h"
#include "pone_map.h"
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

PONE_FUNC(meth_chan) {
    pone_int_t limit;
    PONE_ARG("chan", ":i", &limit);
    return pone_chan_new(world, limit);
}

PONE_FUNC(meth_dd) {
    pone_val* val;
    PONE_ARG("dd", "o", &val);
    THREAD_TRACE("pone_builtin_dd");
    pone_dd(world, val);
    return pone_nil();
}

PONE_FUNC(meth_what) {
    pone_val* val;
    PONE_ARG("what", "o", &val);
    return pone_what(world, val);
}

PONE_FUNC(meth_pthread_self) {
    PONE_ARG("pthread_self", "");
    return pone_int_new(world, (pone_int_t)pthread_self());
}

PONE_FUNC(meth_print) {
    pone_val* val;
    PONE_ARG("print", "o", &val);
    pone_val* str = pone_stringify(world, val);
    fwrite(pone_str_ptr(str), sizeof(char), pone_str_len(str), stdout);
    return pone_nil();
}

PONE_FUNC(meth_say) {
    pone_val* val;
    PONE_ARG("say", "o", &val);
    pone_val* str = pone_str_copy(world, pone_stringify(world, val));
    pone_str_append_c(world, str, "\n", 1);
    fwrite(pone_str_ptr(str), sizeof(char), pone_str_len(str), stdout);
    return pone_nil();
}

PONE_FUNC(meth_getenv) {
    const char* key;
    pone_int_t key_len;
    PONE_ARG("getenv", "s", &key, &key_len);
    const char* len = getenv(key);
    if (len) {
        return pone_str_new_strdup(world, len, strlen(len));
    } else {
        return pone_nil();
    }
}

PONE_FUNC(meth_sleep) {
    pone_val* val;
    PONE_ARG("sleep", "o", &val);
    // TODO Time::HiRes
    pone_int_t i = pone_intify(world, val);
    sleep(i);
    return pone_nil();
}

PONE_FUNC(meth_die) {
    pone_val* msg;
    PONE_ARG("die", "o", &msg);
    pone_throw(world, msg);
    return pone_nil();
}

PONE_FUNC(meth_exit) {
    pone_int_t e = EXIT_SUCCESS;
    PONE_ARG("exit", ":i", &e);
    exit(e);
}

PONE_FUNC(meth_printf) {
    pone_val* fmt = va_arg(args, pone_val*);
    fmt = pone_stringify(world, fmt);

#define PRINTF_BUFSIZ 256
    char fmt_buf[PRINTF_BUFSIZ];
    char dst_buf[PRINTF_BUFSIZ];

    const char* p = pone_str_ptr(fmt);
    const char* end = p + pone_str_len(fmt);
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
                    done = true;
                    break;
                }
                case 'd':
                case 'f': {
                    char fmt = *p;
                    ++p;
                    pone_val* v = va_arg(args, pone_val*);
                    if (p - fmt_p > PRINTF_BUFSIZ - 1) {
                        pone_throw_str(world, "[printf] format string too long");
                    }

                    memcpy(fmt_buf, fmt_p, p - fmt_p);
                    fmt_buf[p - fmt_p] = '\0';

                    int printed;
                    switch (fmt) {
                    case 'f': {
                        pone_num_t n = pone_numify(world, v);
                        printed = snprintf(dst_buf, PRINTF_BUFSIZ - 1, fmt_buf, n);
                        break;
                    }
                    case 'd': {
                        pone_int_t i = pone_intify(world, v);
                        printed = snprintf(dst_buf, PRINTF_BUFSIZ - 1, fmt_buf, i);
                        break;
                    }
                    default:
                        abort();
                    }
                    if (printed > PRINTF_BUFSIZ - 1) {
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

    return pone_nil();
}

PONE_FUNC(meth_internals_dump_lex) {
    pone_lex_dump(world->lex);
    return pone_nil();
}

PONE_FUNC(meth_abort) {
    PONE_ARG("abort", "");
    abort();
}

PONE_FUNC(meth_bless) {
    pone_val* klass;
    pone_val* hash = NULL;
    PONE_ARG("bless(Class $class[, Hash $hash])", "o:o", &klass, &hash);
    pone_val* obj = pone_obj_new(world, klass);
    if (hash) {
        PONE_MAP_FOREACH(hash, key, value, {
            // make $.varname
            pone_val* varname = pone_str_new_strdup(world, "$.", sizeof("$.")-1);
            pone_str_append(world, varname, key);
            pone_str_append_c(world, varname, "\0", sizeof("\0")-1);
            pone_obj_set_ivar(world, obj, pone_str_ptr(varname), value);
        });
    }
    // TODO set default values
    // TODO check required items
    return obj;
}

void pone_builtin_init(pone_world* world) {
    pone_universe* universe = world->universe;
    pone_universe_set_global(universe, "chan", pone_code_new_c(world, meth_chan));
    pone_universe_set_global(universe, "dd", pone_code_new_c(world, meth_dd));
    pone_universe_set_global(universe, "what", pone_code_new_c(world, meth_what));
    pone_universe_set_global(universe, "pthread_self", pone_code_new_c(world, meth_pthread_self));
    pone_universe_set_global(universe, "print", pone_code_new_c(world, meth_print));
    pone_universe_set_global(universe, "say", pone_code_new_c(world, meth_say));
    pone_universe_set_global(universe, "getenv", pone_code_new_c(world, meth_getenv));
    pone_universe_set_global(universe, "sleep", pone_code_new_c(world, meth_sleep));
    pone_universe_set_global(universe, "die", pone_code_new_c(world, meth_die));
    pone_universe_set_global(universe, "exit", pone_code_new_c(world, meth_exit));
    pone_universe_set_global(universe, "printf", pone_code_new_c(world, meth_printf));
    pone_universe_set_global(universe, "INTERNALS__dump_lex", pone_code_new_c(world, meth_internals_dump_lex));
    pone_universe_set_global(universe, "bless", pone_code_new_c(world, meth_bless));
    pone_universe_set_global(universe, "abort", pone_code_new_c(world, meth_abort));
}
