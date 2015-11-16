#include "pone.h" /* PONE_INC */
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

pone_val* pone_builtin_slurp(pone_world* world, pone_val* val) {
    if (pone_str_contains_null(world->universe, val)) {
        pone_throw_str(world, "You can't slurp file. Because file name contains \\0.");
    }

    pone_val* str = pone_str_c_str(world, val);
    FILE *fp = fopen(pone_str_ptr(str), "r");
    if (!fp) {
        pone_throw_str(world, "Cannot open '%s': %s", pone_str_ptr(str), strerror(errno));
    }

    pone_val* retval = pone_str_new(world, "", 0);

    char buf[512];
    while (!feof(fp)) {
        size_t n = fread(buf, 1, 512, fp);
        if (n == 0) {
            fclose(fp);
            pone_throw_str(world, "Cannot read file '%s': %s", pone_str_ptr(str), strerror(errno));
        }
        pone_str_append_c(world, retval, buf, n);
    }

    fclose(fp);

    return retval;
}

pone_val* pone_builtin_dd(pone_world* world, pone_val* val) {
    pone_dd(world, val);
    return pone_nil();
}

pone_val*  pone_builtin_abs(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_INT: {
        pone_int_t i = pone_int_val(val);
        if (i < 0) {
            return pone_int_new(world, -i);
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
    return pone_nil();
}

pone_val* pone_builtin_say(pone_world* world, pone_val* val) {
    pone_val* str = pone_str_copy(world, pone_stringify(world, val));
    pone_str_append_c(world, str, "\n", 1);
    fwrite(pone_str_ptr(str), sizeof(char), pone_str_len(str), stdout);
    return pone_nil();
}

pone_val* pone_builtin_elems(pone_world* world, pone_val* val) {
    return pone_int_new(world, pone_elems(world, val));
}

pone_val* pone_builtin_time(pone_world* world) {
    return pone_int_new(world, time(NULL));
}

pone_val* pone_builtin_getenv(pone_world* world, pone_val* key) {
    pone_val* str = pone_stringify(world, key);
    const char* len = getenv(pone_str_ptr(str));
    if (len) {
        return pone_str_new(world, len, strlen(len));
    } else {
        return pone_nil();
    }
}

pone_val* pone_builtin_sleep(pone_world* world, pone_val* vi) {
    // TODO Time::HiRes
    pone_int_t i = pone_intify(world, vi);
    sleep(i);
    return pone_nil();
}

pone_val* pone_builtin_signal(pone_world* world, pone_val* sig_val, pone_val* code) {
    pone_signal_register_handler(world, pone_intify(world, sig_val), code);
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
                            pone_int_t i = pone_intify(world, v);
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

