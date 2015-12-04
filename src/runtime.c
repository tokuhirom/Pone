#include "pone.h"
#include "pone_module.h"

#define PONE_VERSION "pre-alpha"

PONE_DECLARE_GETTER(meth_filename, "$!filename");
PONE_DECLARE_GETTER(meth_lineno, "$!lineno");
PONE_DECLARE_GETTER(meth_subname, "$!subname");

PONE_FUNC(meth_caller_str) {
    PONE_ARG("Caller#Str", "");

    pone_val* filename = pone_obj_get_ivar(world, self, "$!filename");
    pone_val* lineno = pone_obj_get_ivar(world, self, "$!lineno");
    pone_val* subname = pone_obj_get_ivar(world, self, "$!subname");

    pone_val* buf = pone_str_new_strdup(world, "", 0);
    pone_str_append_c(world, buf, "Caller<file:", strlen("Caller<file:"));
    pone_str_append(world, buf, filename);
    pone_str_append_c(world, buf, ", line:", strlen(", line:"));
    pone_str_append(world, buf, lineno);
    pone_str_append_c(world, buf, ", subname:", strlen(", subname:"));
    pone_str_append(world, buf, subname);
    pone_str_append_c(world, buf, ">", strlen(">"));
    return buf;
}

PONE_FUNC(meth_caller) {
    pone_int_t level = 0;
    PONE_ARG("runtime.caller", ":i", &level);

    pone_int_t l = pone_ary_size(world->caller_stack);
    pone_val* caller = pone_ary_at_pos(world->caller_stack, l - level - 1);

    if (pone_defined(caller)) {
        pone_val* obj = pone_obj_new(world, world->universe->class_caller);
        pone_obj_set_ivar(world, obj, "$!filename", pone_ary_at_pos(caller, 0));
        pone_obj_set_ivar(world, obj, "$!lineno", pone_ary_at_pos(caller, 1));
        pone_obj_set_ivar(world, obj, "$!subname", pone_ary_at_pos(caller, 2));
        return obj;
    } else {
        return pone_nil();
    }
}

void pone_runtime_init(pone_world* world) {
    pone_val* module = pone_module_new(world, "runtime");

    pone_universe_set_global(world->universe, "$PONE_VERSION", pone_str_new_const(world, PONE_VERSION, strlen(PONE_VERSION)));

    {
        pone_val* klass = pone_class_new(world, "Caller", strlen("Caller"));
        pone_add_method_c(world, klass, "filename", strlen("filename"), meth_filename);
        pone_add_method_c(world, klass, "lineno", strlen("lineno"), meth_lineno);
        pone_add_method_c(world, klass, "subname", strlen("subname"), meth_subname);
        pone_add_method_c(world, klass, "Str", strlen("Str"), meth_caller_str);
        pone_module_put(world, module, "Caller", klass);
        world->universe->class_caller = klass;
    }

    pone_module_put(world, module, "caller", pone_code_new_c(world, meth_caller));
    pone_universe_set_global(world->universe, "runtime", module);
}
