#include "pone.h" /* PONE_INC */

#ifdef __GLIBC__
#include <execinfo.h>
#endif

#define PONE_ERR_HANDLERS_INIT 10

void pone_universe_default_err_handler(pone_universe* universe) {
    assert(universe->errvar);
    pone_val* str = pone_to_str(universe, universe->errvar);
    fwrite("\n!!!!!!!!! ( Д ) ..._。..._。 !!!!!!!!!\n\n", 1, strlen("\n!!!!!!!!! ( Д ) ..._。..._。 !!!!!!!!!\n\n"), stderr);
    fwrite(pone_str_ptr(str), 1, pone_str_len(str), stderr);
    fwrite("\n\n", 1, strlen("\n\n"), stderr);

#ifdef __GLIBC__
    {
        void *trace[128];
        int n = backtrace(trace, sizeof(trace) / sizeof(trace[0]));
        backtrace_symbols_fd(trace, n, 1);
    }
#endif

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

#ifdef TRACE_UNIVERSE
    printf("initializing class mu\n");
#endif
    universe->class_mu = pone_init_mu(universe);

#ifdef TRACE_UNIVERSE
    printf("initializing class Class\n");
#endif
    universe->class_class = pone_init_class(universe);

#ifdef TRACE_UNIVERSE
    printf("initializing class Array\n");
#endif
    pone_ary_init(universe);
    assert(universe->class_ary);

#ifdef TRACE_UNIVERSE
    printf("initializing value IterationEnd\n");
#endif
    universe->instance_iteration_end = pone_obj_new(universe, universe->class_mu);

    return universe;
}

void pone_universe_destroy(pone_universe* universe) {
    if (universe->errvar) {
        pone_refcnt_dec(universe, universe->errvar);
    }

    pone_refcnt_dec(universe, universe->instance_iteration_end);
    pone_refcnt_dec(universe, universe->class_ary);
    pone_refcnt_dec(universe, universe->class_class);
    pone_refcnt_dec(universe, universe->class_mu);

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
