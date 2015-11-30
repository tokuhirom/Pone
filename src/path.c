#include "pone.h"
#include "pone_module.h"
#include "pone_opaque.h"
#include "pone_path.h"
#include "pone_time.h"
#include "pone_exc.h"

#include <sys/stat.h>

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

static void stat_finalizer(pone_world* world, pone_val* val) {
    pone_free(world->universe, pone_opaque_ptr(val));
}

PONE_FUNC(meth_path_stat) {
    char* v = pone_str_ptr(pone_str_c_str(world, pone_obj_get_ivar(world, self, "$!path")));
    struct stat* st = pone_malloc(world->universe, sizeof(struct stat));
    if (stat(v, st) == 0) {
        return pone_opaque_new(world, world->universe->class_fileinfo, st, stat_finalizer);
    } else {
        pone_free(world->universe, st);
        pone_world_set_errno(world);
        return pone_nil();
    }
}

#define STAT(key) \
    PONE_FUNC(meth_fileinfo_##key) { \
        struct stat* st = pone_opaque_ptr(self); \
        return pone_int_new(world, st->st_##key); \
    }

STAT(dev)
STAT(ino)
STAT(mode)
STAT(nlink)
STAT(uid)
STAT(gid)
STAT(rdev)
STAT(size)
STAT(blksize)
STAT(blocks)

#define STAT_TIME(key) \
    PONE_FUNC(meth_fileinfo_##key) { \
        struct stat* st = pone_opaque_ptr(self); \
        return pone_localtime_from_epoch(world, st->st_##key); \
    }

STAT_TIME(atime)
STAT_TIME(mtime)
STAT_TIME(ctime)

#undef STAT
#undef STAT_TIME

void pone_path_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_path == NULL);

    universe->class_path = pone_class_new(world, "Path", strlen("Path"));
    pone_add_method_c(world, universe->class_path, "Str", strlen("Str"), meth_path_str);
    pone_add_method_c(world, universe->class_path, "stat", strlen("stat"), meth_path_stat);
    pone_universe_set_global(world->universe, "Path", universe->class_path);

    universe->class_fileinfo = pone_class_new(world, "FileInfo", strlen("FileInfo"));

#define STAT(key) \
    pone_add_method_c(world, universe->class_fileinfo, #key, strlen(#key), meth_fileinfo_##key);

STAT(dev)
STAT(ino)
STAT(mode)
STAT(nlink)
STAT(uid)
STAT(gid)
STAT(rdev)
STAT(size)
STAT(blksize)
STAT(blocks)
STAT(atime)
STAT(mtime)
STAT(ctime)

#undef STAT
    pone_universe_set_global(world->universe, "FileInfo", universe->class_fileinfo);

    pone_universe_set_global(world->universe, "path", pone_code_new_c(world, meth_path_new));
}

