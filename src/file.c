#include "pone.h"
#include "pone_file.h"
#include "pone_opaque.h"
#include <sys/file.h>

#define SELF_FH pone_opaque_ptr(self)

static void finalizer(pone_world* world, pone_val* val) {
    FILE* fp = pone_opaque_ptr(val);
    if (fp) {
        fclose(fp);
    } // closed file contains null pointer.
}

pone_val* pone_file_new(pone_world* world, FILE* fh, bool auto_close) {
    return pone_opaque_new(world, world->universe->class_file, fh, auto_close ? finalizer : NULL);
}

FILE* pone_file_fp(pone_world* world, pone_val* file) {
    return pone_opaque_ptr(pone_obj_get_ivar(world, file, "$!fh"));
}

PONE_FUNC(meth_file_read) {
    pone_int_t nmemb; // buffer size.
    PONE_ARG("File#read", "i", &nmemb);

    char* buf = pone_malloc(world->universe, nmemb);
    size_t r = fread(buf, 1, nmemb, SELF_FH);
    return pone_str_new_allocd(world, buf, r);
}

PONE_FUNC(meth_file_write) {
    pone_val* buf;
    PONE_ARG("File#write", "o", &buf);

    size_t r = fwrite(pone_str_ptr(buf), 1, pone_str_len(buf), SELF_FH);
    assert(r < PONE_INT_MAX);
    return pone_int_new(world, r);
}

PONE_FUNC(meth_file_str) {
    PONE_ARG("File#Str", "");

    return pone_str_new_printf(world, "FILE<%d>", fileno(SELF_FH));
}

PONE_FUNC(meth_file_rewind) {
    PONE_ARG("File#rewind", "");

    rewind(SELF_FH);
    return pone_nil();
}

PONE_FUNC(meth_file_close) {
    PONE_ARG("File#close", "");

    if (fclose(SELF_FH) != 0) {
        pone_world_set_errno(world);
    }
    pone_opaque_set_ptr(self, NULL);
    return pone_nil();
}

PONE_FUNC(meth_file_fileno) {
    PONE_ARG("File#fileno", "");
    return pone_int_new(world, fileno(SELF_FH));
}

PONE_FUNC(meth_file_tell) {
    PONE_ARG("File#fileno", "");
    return pone_int_new(world, ftell(SELF_FH));
}

PONE_FUNC(meth_file_seek) {
    pone_int_t position;
    pone_int_t whence;
    PONE_ARG("File#seek", "ii", &position, &whence);
    if (fseek(SELF_FH, position, whence) == 0) {
        return pone_true();
    } else {
        pone_world_set_errno(world);
        return pone_nil();
    }
}

PONE_FUNC(meth_file_eof) {
    PONE_ARG("File#eof", "");
    return feof(SELF_FH) ? pone_true() : pone_false();
}

PONE_FUNC(meth_file_getc) {
    PONE_ARG("File#getc", "");
    int c = fgetc(SELF_FH);
    if (EOF != c) {
        char cc = (char)c;
        return pone_str_new_strdup(world, &cc, 1);
    } else {
        pone_world_set_errno(world);
        return pone_nil();
    }
}

PONE_FUNC(meth_file_flock) {
    pone_int_t operation;
    PONE_ARG("File#flock", "i", &operation);
    int r = flock(fileno(SELF_FH), operation);
    if (r == 0) {
        return pone_true();
    } else {
        pone_world_set_errno(world);
        return pone_nil();
    }
}

PONE_FUNC(builtin_open) {
    char* fname;
    char* mode = "rb";
    PONE_ARG("File#open", "s:s", &fname, &mode);

    FILE* fh = fopen(fname, mode);
    if (fh) {
        return pone_file_new(world, fh, true);
    } else {
        // TODO autodie support
        pone_world_set_errno(world);
        return pone_nil();
    }
}

PONE_FUNC(builtin_fdopen) {
    pone_int_t fd;
    char* mode;
    PONE_ARG("File#fdopen", "is", &fd, &mode);

    FILE* fh = fdopen(fd, mode);
    if (fh) {
        return pone_file_new(world, fh, false);
    } else {
        // TODO autodie support
        pone_world_set_errno(world);
        return pone_nil();
    }
}

void pone_file_init(pone_world* world) {
    // TODO File#printf
    // TODO File#readline
    // TODO File#lines
    // TODO File#slurp-rest
    pone_val* klass = pone_class_new(world, "File", strlen("File"));
    pone_add_method_c(world, klass, "Str", strlen("Str"), meth_file_str);
    pone_add_method_c(world, klass, "read", strlen("read"), meth_file_read);
    pone_add_method_c(world, klass, "write", strlen("write"), meth_file_write);
    pone_add_method_c(world, klass, "rewind", strlen("rewind"), meth_file_rewind);
    pone_add_method_c(world, klass, "close", strlen("close"), meth_file_close);
    pone_add_method_c(world, klass, "fileno", strlen("fileno"), meth_file_fileno);
    pone_add_method_c(world, klass, "tell", strlen("tell"), meth_file_tell);
    pone_add_method_c(world, klass, "seek", strlen("seek"), meth_file_seek);
    pone_add_method_c(world, klass, "eof", strlen("eof"), meth_file_eof);
    pone_add_method_c(world, klass, "getc", strlen("getc"), meth_file_getc);
    pone_add_method_c(world, klass, "flock", strlen("flock"), meth_file_flock);
    pone_class_compose(world, klass);

    world->universe->class_file = klass;

    pone_universe_set_global(world->universe, "File", klass);

    pone_universe_set_global(world->universe, "open", pone_code_new(world, builtin_open));
    pone_universe_set_global(world->universe, "fdopen", pone_code_new(world, builtin_fdopen));

    // TODO dup2

    pone_universe_set_global(world->universe, "STDIN", pone_file_new(world, stdin, false));
    pone_universe_set_global(world->universe, "STDOUT", pone_file_new(world, stdout, false));
    pone_universe_set_global(world->universe, "STDERR", pone_file_new(world, stderr, false));
    pone_universe_set_global(world->universe, "SEEK_CUR", pone_int_new(world, SEEK_CUR));
    pone_universe_set_global(world->universe, "SEEK_SET", pone_int_new(world, SEEK_SET));
    pone_universe_set_global(world->universe, "SEEK_END", pone_int_new(world, SEEK_END));
    pone_universe_set_global(world->universe, "LOCK_SH", pone_int_new(world, LOCK_SH));
    pone_universe_set_global(world->universe, "LOCK_EX", pone_int_new(world, LOCK_EX));
    pone_universe_set_global(world->universe, "LOCK_UN", pone_int_new(world, LOCK_UN));
}
