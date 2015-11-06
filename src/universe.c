#include "pone.h" /* PONE_INC */

#ifdef __GLIBC__
#include <execinfo.h>
#endif

#define PONE_ERR_HANDLERS_INIT 10

void pone_universe_default_err_handler(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe);
    assert(universe->errvar);
    pone_val* str = pone_stringify(world, universe->errvar);
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

    universe->class_any = pone_init_any(universe);
    universe->class_cool = pone_init_cool(universe);

#ifdef TRACE_UNIVERSE
    printf("initializing class Array\n");
#endif
    pone_ary_init(universe);
    assert(universe->class_ary);

    pone_nil_init(universe);
    pone_int_init(universe);
    pone_str_init(universe);
    pone_num_init(universe);
    pone_bool_init(universe);
    pone_hash_init(universe);
    pone_code_init(universe);
    assert(universe->class_range == NULL);
    pone_range_init(universe);
    pone_regex_init(universe);
    assert(universe->class_io_socket_inet == NULL);
    pone_sock_init(universe);

#ifdef TRACE_UNIVERSE
    printf("initializing value IterationEnd\n");
#endif
    universe->instance_iteration_end = pone_obj_new(universe, universe->class_mu);

    universe->globals = kh_init(str);

#define PUT(key, val) \
    do { \
        int ret; \
        khint_t k = kh_put(str, universe->globals, key, &ret); \
        if (ret == -1) { \
            abort();  \
        } \
        kh_val(universe->globals, k) = val; \
    } while(0)

    PUT("Nil", pone_nil());
    PUT("IO::Socket::INET", universe->class_io_socket_inet);
    PUT("Regex", universe->class_regex);

#undef PUT

    return universe;
}

void pone_universe_destroy(pone_universe* universe) {
    kh_destroy(str, universe->globals);

    if (universe->errvar) {
        pone_refcnt_dec(universe, universe->errvar);
    }

    pone_refcnt_dec(universe, universe->instance_iteration_end);
    pone_refcnt_dec(universe, universe->class_io_socket_inet);
    pone_refcnt_dec(universe, universe->class_match);
    pone_refcnt_dec(universe, universe->class_regex);
    pone_refcnt_dec(universe, universe->class_range);
    pone_refcnt_dec(universe, universe->class_code);
    pone_refcnt_dec(universe, universe->class_hash);
    pone_refcnt_dec(universe, universe->class_bool);
    pone_refcnt_dec(universe, universe->class_num);
    pone_refcnt_dec(universe, universe->class_int);
    pone_refcnt_dec(universe, universe->class_str);
    pone_refcnt_dec(universe, universe->class_nil);
    pone_refcnt_dec(universe, universe->class_ary);
    pone_refcnt_dec(universe, universe->class_any);
    pone_refcnt_dec(universe, universe->class_cool);
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

