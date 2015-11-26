#include "pone.h"

#ifdef __GLIBC__
#include <execinfo.h>
#endif

jmp_buf* pone_exc_handler_push(pone_world* world) {
    if (world->err_handler_idx == world->err_handler_max) {
        world->err_handler_max *= 2;
        world->err_handlers = realloc(world->err_handlers, sizeof(jmp_buf) * world->err_handler_max);
        if (!world->err_handlers) {
            fprintf(stderr, "can't alloc mem\n");
            exit(1);
        }

        world->err_handler_lexs = realloc(world->err_handler_lexs, sizeof(pone_world*) * world->err_handler_max);
        if (!world->err_handler_lexs) {
            fprintf(stderr, "can't alloc mem\n");
            exit(1);
        }
    }

    world->err_handler_lexs[world->err_handler_idx + 1] = world->lex;
    return &(world->err_handlers[++world->err_handler_idx]);
}

void pone_exc_handler_pop(pone_world* world) {
    assert(world->universe);
    assert(world->err_handler_idx >= 0);
    world->err_handler_idx--;
}

void pone_throw_str(pone_world* world, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    pone_val* v = pone_str_new_vprintf(world, fmt, args);
    va_end(args);
    pone_throw(world, v);
}

// TODO rename to pone_throw
void pone_throw(pone_world* world, pone_val* val) {
    assert(val);

    // save error information to $!
    world->errvar = val;

    // back to the lex
    world->lex = world->err_handler_lexs[world->err_handler_idx];

    EXC_TRACE("throwing exc");

#ifdef __GLIBC__
    {
        void* buffer[128];
        int nptrs = backtrace(buffer, 128);

        char** strings = backtrace_symbols(buffer, nptrs);
        if (strings == NULL) {
            perror("backtrace_symbols");
        } else {
            pone_val* ary = pone_ary_new(world, 0);
            for (int j = 0; j < nptrs; j++) {
                pone_ary_push(world->universe, ary, pone_str_new_strdup(world, strings[j], strlen(strings[j])));
            }

            free(strings);

            world->stacktrace = ary;
        }
    }
#endif

    // jmp to exception handler
    longjmp(world->err_handlers[world->err_handler_idx--], 1);
}

void pone_warn_str(pone_world* world, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n"); // TODO: show line number
}

pone_val* pone_try(pone_world* world, pone_val* code) {
    assert(pone_type(code) == PONE_CODE);

    if (setjmp(*(pone_exc_handler_push(world)))) {
        return pone_nil();
    } else {
        pone_val* v = pone_code_call(world, code, pone_nil(), 0);
        pone_exc_handler_pop(world);
        return v;
    }
}

// get $@
pone_val* pone_errvar(pone_world* world) {
    return world->errvar;
}
