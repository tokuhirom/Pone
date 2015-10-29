#include "pone.h" /* PONE_INC */

#define PONE_ERR_HANDLERS_INIT 10

void pone_universe_default_err_handler(pone_universe* universe) {
    assert(universe->errvar);
    pone_val* str = pone_to_str(universe, universe->errvar);
    fwrite("\n!!!!!!!!! ( Д ) ..._。..._。 !!!!!!!!!\n\n", 1, strlen("\n!!!!!!!!! ( Д ) ..._。..._。 !!!!!!!!!\n\n"), stderr);
    fwrite(pone_string_ptr(str), 1, pone_string_len(str), stderr);
    fwrite("\n\n", 1, strlen("\n\n"), stderr);
    exit(1);
}

pone_universe* pone_universe_init() {
    pone_universe*universe = malloc(sizeof(pone_universe));
    if (!universe) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    memset(universe, 0, sizeof(pone_universe));
    universe->arena_last = universe->arena_head = malloc(sizeof(pone_arena));
    if (!universe->arena_last) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    memset(universe->arena_last, 0, sizeof(pone_arena));
    universe->errvar = pone_nil();

    universe->err_handlers = malloc(sizeof(jmp_buf)*PONE_ERR_HANDLERS_INIT);
    if (!universe->err_handlers) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    universe->err_handler_worlds = malloc(sizeof(pone_world*)*PONE_ERR_HANDLERS_INIT);
    if (!universe->err_handler_worlds) {
        fprintf(stderr, "cannot allocate memory\n");
        exit(1);
    }
    universe->err_handler_idx = 0;
    universe->err_handler_max = PONE_ERR_HANDLERS_INIT;

    pone_ary_iter_init(universe);
    assert(universe->class_ary_iter);

    return universe;
}

void pone_universe_destroy(pone_universe* universe) {
    if (universe->errvar) {
        pone_refcnt_dec(universe, universe->errvar);
    }

    assert(pone_refcnt(universe->class_ary_iter) == 1);
    pone_refcnt_dec(universe, universe->class_ary_iter);

    pone_arena* a = universe->arena_head;
    while (a) {
        pone_arena* next = a->next;
        free(a);
        a = next;
    }
    free(universe->err_handler_worlds);
    free(universe->err_handlers);
    free(universe);
}

