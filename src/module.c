// Module is a core feature in Pone.
#include "pone.h"
#include "pone_file.h"
#include "pone_exc.h"
#include "pone_compile.h"
#include "dlfcn.h"
#include <sys/stat.h>

static void export_module(pone_world* world, const char* name, pone_int_t name_len, pone_val* module) {
    if (name_len == 1 && name[0] == '.') {
        const char* k;
        pone_val* v;
        kh_foreach(module->as.obj.ivar, k, v, {
            assert(v);
            // TODO we may need to strdup `k`
            pone_assign(world, 0, k, v);
        });
    } else {
        pone_assign(world, 0, name, module);
    }
}

pone_val* pone_module_new(pone_world* world, const char* name) {
    pone_val* obj = pone_obj_new(world, world->universe->class_module);
    pone_obj_set_ivar(world, obj, "$!name", pone_str_new_strdup(world, name, strlen(name)));
    return obj;
}

void pone_module_put(pone_world* world, pone_val* self, const char* name, pone_val* val) {
    pone_obj_set_ivar(world, self, name, val);
}

PONE_FUNC(meth_module_str) {
    PONE_ARG("Module#Str", "");
    pone_val* v = pone_str_new_strdup(world, "Module:", strlen("Module:"));
    pone_str_append(world, v, pone_obj_get_ivar(world, self, "$!name"));
    return v;
}

PONE_FUNC(meth_module_can) {
    char* name;
    pone_int_t name_len;
    PONE_ARG("Module#can", "s", &name, &name_len);
    return pone_obj_get_ivar(world, self, name);
}

void pone_module_init(pone_world* world) {
    pone_val* module = pone_class_new(world, "Module", strlen("Module"));
    pone_add_method_c(world, module, "Str", strlen("Str"), meth_module_str);
    pone_add_method_c(world, module, "can", strlen("can"), meth_module_can);

    world->universe->class_module = module;
}

// entry point name is
//
// my $funcname = $pkg;
// $funcname =~ s!/!_!g;
// $funcname = "PONE_DLL_$funcname";
static bool load_module(pone_world* world, const char* from, const char* name, const char* as, pone_int_t as_len) {
#if defined(__linux__)
#define PONE_DLL_EXT "so"
#elif defined(__APPLE__)
#define PONE_DLL_EXT "dylib"
#else
#error "Unsupported operating system"
#endif

    pone_val* fullpath_v = pone_str_new_printf(world, "%s/%s.%s", from, name, PONE_DLL_EXT);
    const char* fullpath = pone_str_ptr(pone_str_c_str(world, fullpath_v));
    struct stat stat_buf;
    if (stat(fullpath, &stat_buf) != 0) {
        return false;
    }

    pone_int_t name_len = strlen(name);
    // s!/!_!g;
    pone_val* funcname_tail = pone_str_new_strdup(world, name, name_len);
    char* p = pone_str_ptr(funcname_tail);
    char* p_end = p + pone_str_len(funcname_tail);
    while (p != p_end) {
        if (*p == '/') {
            *p = '_';
        }
        p++;
    }

    pone_val* funcname = pone_str_new_strdup(world, "PONE_DLL_", strlen("PONE_DLL_"));
    pone_str_append(world, funcname, funcname_tail);

    void* handle = dlopen(fullpath, RTLD_LAZY);
    if (!handle) {
        pone_throw_str(world, "Could not load module %s: %s", fullpath, dlerror());
    }
    pone_loadfunc_t pone_load = dlsym(handle, pone_str_ptr(
                                                  pone_str_c_str(world, funcname)));

    char* error;
    if ((error = dlerror()) != NULL) {
        printf("%s\n", error);
        // TODO: dlclose
        pone_throw_str(world, "Could not load module %s(dlsym): %s", fullpath, error);
    }

    pone_val* module = pone_module_new(world, name);
    pone_load(world, module);

    export_module(world, as, as_len, module);
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
static bool try_load_pn(pone_world* world, pone_val* dir, const char* name, const char* as, pone_int_t as_len) {
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

    pone_val* module = pone_code_call(world, __FILE__, __LINE__, __func__, code, pone_nil(), 0);

    world->lex = orig_lex;

    export_module(world, as, as_len, module);
    return true;
}

void pone_use(pone_world* world, const char* name, const char* as, pone_int_t as_len) {
    // scan $*INC
    pone_val* inc = world->universe->inc;
    for (pone_int_t i = 0; i < pone_ary_size(inc); ++i) {
        pone_val* dir = pone_ary_at_pos(inc, i);

        // try ${dir}/${name}.pn
        if (try_load_pn(world, dir, name, as, as_len)) {
            return;
        }

        // try ${dir}/${name}.so
        const char* from = pone_str_ptr(pone_str_c_str(world, pone_stringify(world, dir)));
        if (load_module(world, from, name, as, as_len)) {
            return;
        }
    }

    pone_val* msg = pone_str_new_printf(world, "Could not load module '%s': no such module in:\n\n", name);
    for (pone_int_t i = 0; i < pone_ary_size(inc); ++i) {
        pone_str_append_c(world, msg, "    - ", strlen("    - "));
        pone_str_append(world, msg, pone_ary_at_pos(inc, i));
        pone_str_append_c(world, msg, "\n", strlen("\n"));
    }
    pone_throw(world, msg);
}
