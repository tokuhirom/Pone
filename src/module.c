// Module is a core feature in Pone.
#include "pone.h"
#include "dlfcn.h"
#include <sys/stat.h>

pone_val* pone_module_new(pone_world* world, const char *name) {
    pone_val*obj = pone_obj_new(world, world->universe->class_module);
    pone_obj_set_ivar(world, obj, "$!name", pone_str_new_strdup(world, name, strlen(name)));
    return obj;
}

void pone_module_put(pone_world* world, pone_val* self, const char *name, pone_val* val) {
    pone_obj_set_ivar(world, self, name, val);
}

PONE_FUNC(meth_module_str) {
    pone_val* v = pone_str_new_strdup(world, "Module:", strlen("Module:"));
    pone_str_append(world, v, pone_obj_get_ivar(world, self, "$!name"));
    return v;
}

void pone_module_init(pone_world* world) {
    pone_val* module = pone_class_new(world, "Module", strlen("Module"));
    pone_add_method_c(world, module, "Str", strlen("Str"), meth_module_str);
    pone_class_compose(world, module);
    world->universe->class_module = module;
}

// entry point name is
//
// my $funcname = $pkg;
// $funcname =~ s!/!_!g;
// $funcname = "PONE_DL_$funcname";
static bool load_module(pone_world* world, const char* from, const char *name, const char* as) {
    pone_val* fullpath_v = pone_str_new_printf(world, "%s/%s.so", from, name);
    const char* fullpath = pone_str_ptr(pone_str_c_str(world, fullpath_v));
    struct stat stat_buf;
    if (stat(fullpath, &stat_buf) != 0) {
        return false;
    }

    void* handle = dlopen(fullpath, RTLD_LAZY);
    if (!handle) {
        pone_throw_str(world, "Could not load module %s: %s", fullpath, dlerror());
    }
    pone_loadfunc_t pone_load = dlsym(handle, "PONE_DLL_io_socket_inet");

    char* error;
    if ((error = dlerror()) != NULL)  {
        dlclose(handle);
        pone_throw_str(world, "Could not load module %s: %s", fullpath, dlerror());
    }

    pone_val* module = pone_module_new(world, name);
    pone_load(world, module);

    pone_assign(world, 0, as, module);
    return true;
}

void pone_use(pone_world* world, const char *name, const char* as) {
    pone_val* inc = world->universe->inc;
    for (pone_int_t i=0; i<pone_ary_elems(inc); ++i) {
        const char* from = pone_str_ptr(pone_str_c_str(world, pone_stringify(world, pone_ary_at_pos(inc, i))));
        if (load_module(world, from, name, as)) {
            return;
        }
    }
    // TODO dump $*INC
    pone_throw_str(world, "Could not load module %s: no such module.", name);
}

