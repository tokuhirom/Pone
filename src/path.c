#include "pone.h"
#include "pone_module.h"
#include "pone_opaque.h"
#include "pone_path.h"
#include "pone_time.h"
#include "pone_exc.h"
#include "pone_stat.h"

pone_val* pone_path_new(pone_world* world, pone_val* path) {
    pone_val* obj = pone_obj_new(world, world->universe->class_path);
    pone_obj_set_ivar(world, obj, "$!path", pone_stringify(world, path));
    return obj;
}

PONE_FUNC(meth_path_new) {
    pone_val* path;
    PONE_ARG("path(Str $path)", "o", &path);
    return pone_path_new(world, path);
}

PONE_FUNC(meth_path_str) {
    return pone_obj_get_ivar(world, self, "$!path");
}

PONE_FUNC(meth_path_stat) {
    char* v = pone_str_ptr(pone_str_c_str(world, pone_obj_get_ivar(world, self, "$!path")));
    return pone_stat(world, v);
}

void pone_path_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_path == NULL);

    universe->class_path = pone_class_new(world, "Path", strlen("Path"));
    pone_add_method_c(world, universe->class_path, "Str", strlen("Str"), meth_path_str);
    pone_add_method_c(world, universe->class_path, "stat", strlen("stat"), meth_path_stat);
    pone_universe_set_global(world->universe, "Path", universe->class_path);

    pone_val* module = pone_module_new(world, "path");
    pone_module_put(world, module, "new", pone_code_new_c(world, meth_path_new));
    pone_universe_set_global(world->universe, "path", module);
}

