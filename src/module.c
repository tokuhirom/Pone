// Module is a core feature in Pone.
#include "pone.h"
#include "pone_file.h"
#include "pone_compile.h"
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

pone_val* pone_module_from_lex(pone_world* world, const char* module_name) {
    pone_val* module = pone_module_new(world, module_name);
    pone_val* lex = world->lex;
    const char* k;
    pone_val* v;
    kh_foreach(lex->as.lex.map, k, v, {
        pone_val* klass = pone_what(world, v);
        // export only class and code.
        // TODO support 'my class' and 'my sub'. it means v->flags |= PONE_FLAGS_CLASS_MY;
        if (klass == world->universe->class_class || klass == world->universe->class_code) {
            pone_module_put(world, module, k, v);
        }
    });
    return module;
}

// try load ${dir}/${name}.pn.
static bool try_load_pn(pone_world* world, pone_val* dir, const char* name, const char* as) {
    pone_val* path = pone_stringify(world, dir);
    pone_str_append_c(world, path, "/", strlen("/"));
    pone_str_append_c(world, path, name, strlen(name));
    pone_str_append_c(world, path, ".pn", strlen(".pn"));
    const char* path_c = pone_str_ptr(pone_str_c_str(world, path));

    MODULE_TRACE("Loading %s", path_c);

    FILE* fp = fopen(path_c, "r");
    if (!fp) {
        int e = errno;
        struct stat stat_buf;
        if (stat(path_c, &stat_buf) != 0) {
            // file does not exist.
            return false;
        } else {
            // file exists but can't open. it's a problem. it means permission errror or something.
            pone_throw_str(world, "Cannot open %s: %s", path_c, strerror(e));
        }
    }

    // manage file handle by GC.
    (void)pone_file_new(world, fp, true);
    pone_val* code = pone_compile_fp(world, fp, path_c);

    // save original lex.
    pone_val* orig_lex = world->lex;
    pone_save_tmp(world, orig_lex);
    // create new lex from Code's saved lex.
    world->lex = NULL;

    pone_val* module = pone_code_call(world, code, pone_nil(), 0);

    world->lex = orig_lex;

    pone_assign(world, 0, as, module);
    return true;
}

void pone_use(pone_world* world, const char *name, const char* as) {
    // scan $*INC
    pone_val* inc = world->universe->inc;
    for (pone_int_t i=0; i<pone_ary_elems(inc); ++i) {
        pone_val* dir = pone_ary_at_pos(inc, i);

        // try ${dir}/${name}.pn
        if (try_load_pn(world, dir, name, as)) {
            return;
        }

        // try ${dir}/${name}.so
        const char* from = pone_str_ptr(pone_str_c_str(world, pone_stringify(world, dir)));
        if (load_module(world, from, name, as)) {
            return;
        }
    }

    pone_val* msg = pone_str_new_printf(world, "Could not load module '%s': no such module in:\n\n", name);
    for (pone_int_t i=0; i<pone_ary_elems(inc); ++i) {
        pone_str_append_c(world, msg, "    ", strlen("    "));
        pone_str_append(world, msg, pone_ary_at_pos(inc, i));
    }
    pone_throw(world, msg);
}

