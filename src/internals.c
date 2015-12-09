#include "pone.h"
#include "pone_module.h"

// These APIs are internal API.
PONE_FUNC(meth_set_global) {
    const char* name;
    pone_int_t name_len;
    pone_val* val;
    PONE_ARG("internals.set_global", "so", &name, &name_len, &val);
    pone_universe_set_global(world->universe, name, val);
    printf("SeT %s\n", name);
    return pone_nil();
}

PONE_FUNC(meth_dump_lex) {
    pone_lex_dump(world->lex);
    return pone_nil();
}

void pone_internals_init(pone_world* world) {
    pone_val* internals = pone_module_new(world, "internals");
    pone_module_put(world, internals, "set_global", pone_code_new(world, meth_set_global));
    pone_module_put(world, internals, "dump_lex", pone_code_new(world, meth_dump_lex));

    pone_universe_set_global(world->universe, "internals", internals);
}
