#include "pone.h"

PONE_FUNC(meth_getpid) {
    PONE_ARG("OS#getpid", "");
    return pone_int_new(world, getpid());
}

PONE_FUNC(meth_getwd) {
    PONE_ARG("OS#getwd", "");
    char buf[PATH_MAX];
    if (getcwd(buf, PATH_MAX) == NULL) {
        pone_world_set_errno(world);
        return pone_nil();
    }
    return pone_str_new(world, buf, strlen(buf));
}

void pone_os_init(pone_world* world) {
    pone_val* klass = pone_class_new(world, "OS", strlen("OS"));
    PONE_REG_METHOD(klass, "getpid", meth_getpid);
    PONE_REG_METHOD(klass, "getwd", meth_getwd);
    pone_obj_set_ivar(world, klass, "is_win",
#if defined(_WIN32) || defined(_WIN64)
            pone_true()
#else
            pone_false()
#endif
            );
    pone_universe_set_global(world->universe, "OS", klass);
}

