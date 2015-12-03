#include "pone.h"
#include "pone_module.h"
#include "pone_opaque.h"
#include "pone_time.h"
#include "pone_exc.h"

#include <sys/stat.h>

static void stat_finalizer(pone_world* world, pone_val* val) {
    pone_free(world, pone_opaque_ptr(val));
}

pone_val* pone_stat(pone_world* world, char * filename) {
    struct stat* st = pone_malloc(world, sizeof(struct stat));
    if (stat(filename, st) == 0) {
        return pone_opaque_new(world, world->universe->class_stat, st, stat_finalizer);
    } else {
        pone_free(world, st);
        pone_world_set_errno(world);
        return pone_nil();
    }
}

#define STAT(key)                                 \
    PONE_FUNC(meth_fileinfo_##key) {              \
        struct stat* st = pone_opaque_ptr(self);  \
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

#define STAT_TIME(key)                                       \
    PONE_FUNC(meth_fileinfo_##key) {                         \
        struct stat* st = pone_opaque_ptr(self);             \
        return pone_time_from_epoch(world, st->st_##key, 0); \
    }

STAT_TIME(atime)
STAT_TIME(mtime)
STAT_TIME(ctime)

#undef STAT
#undef STAT_TIME

void pone_stat_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_stat == NULL);

    universe->class_stat = pone_class_new(world, "Stat", strlen("Stat"));

#define STAT(key) \
    pone_add_method_c(world, universe->class_stat, #key, strlen(#key), meth_fileinfo_##key);

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
    pone_universe_set_global(world->universe, "Stat", universe->class_stat);
}

