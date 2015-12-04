#include "pone.h"
#include "pone_exc.h"

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
    pone_val* exc = pone_obj_new(world, world->universe->class_x_adhoc);
    pone_obj_set_ivar(world, exc, "$!payload", v);
    pone_throw(world, exc);
}

// TODO rename to pone_throw
__attribute__((noreturn)) void pone_throw(pone_world* world, pone_val* val) {
    assert(val);

    // save error information to $!
    world->errvar = val;

    // back to the lex
    world->lex = world->err_handler_lexs[world->err_handler_idx];

    // TODO rewind caller stack

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
        pone_val* v = pone_code_call(world, __FILE__, __LINE__, __func__, code, pone_nil(), 0);
        pone_exc_handler_pop(world);
        return v;
    }
}

PONE_FUNC(meth_exc_message) {
    PONE_ARG("#message", "");
    return pone_obj_get_ivar(world, pone_what(world, self), "$!message");
}

pone_val* pone_exc_class_new_simple(pone_world* world, const char* name, pone_int_t name_len, const char* message) {
    pone_val* klass = pone_class_new(world, name, strlen(name));
    pone_add_method_c(world, klass, "message", strlen("message"), meth_exc_message);
    pone_add_method_c(world, klass, "Str", strlen("Str"), meth_exc_message);
    pone_obj_set_ivar(world, klass, "$!message", pone_str_new_strdup(world, message, strlen(message)));
    return klass;
}

// get $@
pone_val* pone_errvar(pone_world* world) {
    return world->errvar;
}

PONE_FUNC(exc_adhoc_str) {
    PONE_ARG("X::AdHoc#Str", "");
    pone_val* payload = pone_obj_get_ivar(world, self, "$!payload");
    return pone_stringify(world, payload);
}

void pone_exc_init(pone_world* world) {
    pone_val* klass = pone_class_new(world, "X::AdHoc", strlen("X::AdHoc"));
    pone_add_method_c(world, klass, "Str", strlen("Str"), exc_adhoc_str);

    world->universe->class_x_adhoc = klass;
    pone_universe_set_global(world->universe, "X::AdHoc", klass);
}
