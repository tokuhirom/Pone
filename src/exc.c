#include "pone.h"

jmp_buf* pone_exc_handler_push(pone_world* world) {
    if (world->universe->err_handler_idx == world->universe->err_handler_max) {
        world->universe->err_handler_max *= 2;
        world->universe->err_handlers = realloc(world->universe->err_handlers, sizeof(jmp_buf)*world->universe->err_handler_max);
        if (!world->universe->err_handlers) {
            fprintf(stderr, "can't alloc mem\n");
            exit(1);
        }

        world->universe->err_handler_worlds = realloc(world->universe->err_handler_worlds, sizeof(pone_world*)*world->universe->err_handler_max);
        if (!world->universe->err_handler_worlds) {
            fprintf(stderr, "can't alloc mem\n");
            exit(1);
        }
    }

    world->universe->err_handler_worlds[world->universe->err_handler_idx+1] = world;
    return &(world->universe->err_handlers[++world->universe->err_handler_idx]);
}

void pone_exc_handler_pop(pone_world* world) {
    assert(world->universe);
    assert(world->universe->err_handler_idx >= 0);
    world->universe->err_handler_idx--;
}

void pone_throw_str(pone_world* world, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    pone_val* v = pone_mortalize(world, pone_str_new_vprintf(world->universe, fmt, args));
    va_end(args);
    pone_throw(world, v);
}

// TODO rename to pone_throw
void pone_throw(pone_world* world, pone_val* val) {
    assert(val);

    pone_universe* universe = world->universe;

    if (universe->errvar) {
        pone_refcnt_dec(universe, universe->errvar);
    }

    // save error information to $!
    universe->errvar = val;
    pone_refcnt_inc(universe, val);

    pone_world* target_world = universe->err_handler_worlds[universe->err_handler_idx];

    // exit from this scope
    while (world != target_world) {
        pone_world* parent = world->parent;
        pone_destroy_world(world);
        world = parent;
    }

    // jmp to exception handler
    longjmp(universe->err_handlers[universe->err_handler_idx--], 1);
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

// get $!
// $! is equivalent to $@ in Perl5
pone_val* pone_errvar(pone_world* world) {
    return world->universe->errvar;
}

