#include "pone.h"

PONE_FUNC(meth_getwd) {
    PONE_ARG("OS#getwd", "");
    char buf[PATH_MAX];
    if (getcwd(buf, PATH_MAX) == NULL) {
        pone_world_set_errno(world);
        return pone_nil();
    }
    return pone_str_new_strdup(world, buf, strlen(buf));
}

#define PONE_REG_METHOD(name, meth)  \
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
    pone_val* klass = pone_class_new(world, "OS", strlen("OS"));
    pone_add_method_c(world, klass, "getwd", strlen("getwd"), meth_getwd);
    pone_add_method_c(world, klass, "is_win", strlen("is_win"), meth_is_win);
    pone_universe_set_global(world->universe, "OS", klass);
}

