#include "pone.h"

static int min_args(const char* spec) {
    int n = 0;
    while (*spec) {
        if (*spec == ':') {
            break;
        }
        n++;
        spec++;
    }
    return n;
}

static int max_args(const char* spec) {
    int n = 0;
    while (*spec) {
        if (*spec != ':') {
            n++;
        }
        spec++;
    }
    return n;
}

void pone_arg(pone_world* world, const char* name, int nargs, va_list args, const char* spec, ...) {
    va_list vars;

    va_start(vars, spec);

    const char* p = spec;

    bool option = false;
    int used = 0;
    while (*p) {

        switch (*p) {
        case 'o': // object.
        {
            if (nargs <= used) {
                if (option) {
                    break;
                } else {
                    pone_throw_str(world, "%s: expected %d~%d opts but you passed %d opts.", name, min_args(spec), max_args(spec), nargs);
                }
            }
            pone_val** p = va_arg(vars, pone_val**);
            *p = va_arg(args, pone_val*);
            used++;
            break;
        }
        case 's': // string
        {
            if (nargs <= used) {
                if (option) {
                    break;
                } else {
                    pone_throw_str(world, "%s: expected %d~%d opts but you passed %d opts.", name, min_args(spec), max_args(spec), nargs);
                }
            }
            const char** p = va_arg(vars, const char**);
            *p = pone_str_ptr(pone_str_c_str(world, pone_stringify(world, va_arg(args, pone_val*))));
            used++;
            break;
        }
        case 'i': // pone_int_t
        {
            if (nargs <= used) {
                if (option) {
                    break;
                } else {
                    pone_throw_str(world, "%s: expected %d~%d opts but you passed %d opts.", name, min_args(spec), max_args(spec), nargs);
                }
            }
            pone_int_t* p = va_arg(vars, pone_int_t*);
            *p = pone_intify(world, va_arg(args, pone_val*));
            used++;
            break;
        }
        case ':': // option
        {
            option = true;
            break;
        }
        default:
            pone_throw_str(world, "%s: Unknown argument spec char: '%c'", name, *spec);
            break;
        }
        p++;
    }

    va_end(vars);

    if (used < min_args(spec) || max_args(spec) < used) {
        pone_throw_str(world, "%s: expected %d~%d opts but you passed %d opts.", name, min_args(spec), max_args(spec), nargs);
    }
}
