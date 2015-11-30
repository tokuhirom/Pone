#include "pone.h"
#include "pone_file.h"
#include "pone_exc.h"
#include "pone_path.h"
#include "pone_tmpfile.h"

// TODO unlink file

static const char* get_tmpdir() {
    const char* t = getenv("TMPDIR");
    if (t) {
        return t;
    }
#ifdef P_tmpdir
    return P_tmpdir;
#else
    return "/tmp";
#endif
}

// initialize tmpfile
pone_val* pone_tmpfile_new(pone_world* world) {
    pone_val* self = pone_obj_new(world, world->universe->class_tmpfile);
    const char* tmp = get_tmpdir();
    pone_val* path = pone_str_new_strdup(world, tmp, strlen(tmp));
    pone_str_append_c(world, path, "/pone_XXXXXX", strlen("/pone_XXXXXX"));
    path = pone_str_c_str(world, path);
    // mkstemp: POSIX.1-2001.
    char* ptr = pone_str_ptr(path);
    int fd = mkstemp(ptr);
    if (fd < 0) {
        pone_throw_str(world, "mkstemp: cannot create tmpfile: %s", strerror(errno));
    }
    pone_obj_set_ivar(world, self, "$!path", path);
    FILE* fp = fdopen(fd, "rw");
    if (!fp) {
        pone_throw_str(world, "fdopen: cannot create tmpfile: %s", strerror(errno));
    }

    pone_obj_set_ivar(world, self, "$!file", pone_file_new(world, fp, true));

    return self;
}

const char* pone_tmpfile_path_c(pone_world* world, pone_val* self) {
    return pone_str_ptr(pone_str_c_str(world, pone_obj_get_ivar(world, self, "$!path")));
}

FILE* pone_tmpfile_fp(pone_world* world, pone_val* self) {
    return pone_file_fp(world, pone_obj_get_ivar(world, self, "$!file"));
}

PONE_FUNC(meth_new) {
    PONE_ARG("tmpfile", "");
    return pone_tmpfile_new(world);
}

PONE_FUNC(meth_path) {
    PONE_ARG("TempFile#Path", "");
    return pone_path_new(world, pone_obj_get_ivar(world, self, "$!path"));
}

PONE_FUNC(meth_file) {
    PONE_ARG("TempFile#file", "");
    return pone_obj_get_ivar(world, self, "$!file");
}

// create tmpfile module
void pone_tmpfile_init(pone_world* world) {
    pone_val* klass = pone_class_new(world, "TmpFile", strlen("TmpFile"));
    pone_add_method_c(world, klass, "Path", strlen("Path"), meth_path);
    pone_add_method_c(world, klass, "file", strlen("file"), meth_file);

    world->universe->class_tmpfile = klass;
    pone_universe_set_global(world->universe, "tmpfile", pone_code_new_c(world, meth_new));
}
