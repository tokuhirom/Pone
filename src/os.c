#include "pone.h"
#include "pone_module.h"

PONE_FUNC(meth_getwd) {
    PONE_ARG("getcwd", "");
    char buf[PATH_MAX];
    if (getcwd(buf, PATH_MAX) == NULL) {
        pone_world_set_errno(world);
        return pone_nil();
    }
    return pone_str_new_strdup(world, buf, strlen(buf));
}

#define PONE_REG_METHOD(name, meth) \
    pone_add_method_c(world, klass, name, strlen(name), meth);

PONE_FUNC(meth_is_win) {
    PONE_ARG("OS#is_win", "");
#if defined(_WIN32) || defined(_WIN64)
    return pone_true();
#else
    return pone_false();
#endif
}

void pone_os_init(pone_world* world) {
    pone_val* module = pone_module_new(world, "os");
    pone_module_put(world, module, "is_win", pone_code_new(world, meth_is_win));
    pone_universe_set_global(world->universe, "os", module);

    // builtin getcwd() function
    pone_universe_set_global(world->universe, "getcwd", pone_code_new(world, meth_getwd));
}
