#include "pone.h"
#include "pone_file.h"
#include "pone_opaque.h"
#include "pone_module.h"
#include "pone_stat.h"
#include "pone_exc.h"
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

    pone_val* buf = pone_bytes_new_malloc(world, nmemb);
    size_t r = fread(pone_str_ptr(buf), 1, nmemb, SELF_FH);
    pone_bytes_truncate(buf, r);
    return buf;
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

static void pone_file_close(pone_world* world, pone_val* self) {
    if (fclose(SELF_FH) != 0) {
        pone_world_set_errno(world);
    }
    pone_opaque_set_ptr(self, NULL);
}

PONE_FUNC(meth_file_close) {
    PONE_ARG("File#close", "");

    pone_file_close(world, self);
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

PONE_FUNC(meth_file_stat) {
    PONE_ARG("File#stat", "");
    return pone_fstat(world, fileno(SELF_FH));
}

PONE_FUNC(meth_file_list_iterator_pull_one) {
    PONE_ARG("FileSeqIterator#pull-one", "");

    pone_val* file = pone_obj_get_ivar(world, self, "$!file");
    pone_val* retval = pone_bytes_new_strdup(world, "", 0);
    FILE* fp = pone_opaque_ptr(file);
    char buf[512];
    while (!feof(fp)) {
        if (fgets(buf, sizeof(buf), fp)) {
            size_t len = strlen(buf);
            pone_str_append_c(world, retval, buf, len);
            if (buf[len - 1] == '\n') {
                return retval;
            }
        }
    }
    if (pone_str_len(retval)) {
        return retval;
    } else {
        return world->universe->instance_iteration_end;
    }
}

PONE_FUNC(meth_file_seq_iterator) {
    PONE_ARG("FileSeq#iterator", "");
    // FileSeqIterator
    pone_val* klass = pone_obj_get_ivar(world, pone_what(world, self), "FileSeqIterator");
    pone_val* file = pone_obj_get_ivar(world, self, "$!file");
    pone_val* iter = pone_obj_new(world, klass);
    pone_obj_set_ivar(world, iter, "$!file", file);
    return iter;
}

// convert lines to list
PONE_FUNC(meth_file_lines) {
    PONE_ARG("File#lines", "");
    pone_val* klass = pone_obj_get_ivar(world, pone_what(world, self), "FileSeq");
    assert(klass);
    pone_val* list = pone_obj_new(world, klass);
    pone_obj_set_ivar(world, list, "$!file", self);
    return list;
}

PONE_FUNC(meth_open) {
    char* fname;
    pone_int_t fname_len;
    char* mode = "rb";
    pone_int_t mode_len;
    PONE_ARG("File#open", "s:s", &fname, &fname_len, &mode, &mode_len);

    FILE* fh = fopen(fname, mode);
    if (fh) {
        return pone_file_new(world, fh, true);
    } else {
        // TODO autodie support
        pone_world_set_errno(world);
        return pone_nil();
    }
}

PONE_FUNC(meth_fdopen) {
    pone_int_t fd;
    char* mode;
    pone_int_t mode_len;
    PONE_ARG("File#fdopen", "is", &fd, &mode, &mode_len);

    FILE* fh = fdopen(fd, mode);
    if (fh) {
        return pone_file_new(world, fh, false);
    } else {
        // TODO autodie support
        pone_world_set_errno(world);
        return pone_nil();
    }
}

PONE_FUNC(meth_file_slurp_rest) {
    PONE_ARG("File#slurp_rest", "");

    FILE* fp = SELF_FH;

    pone_val* retval = pone_str_new_strdup(world, "", 0);

    char buf[512];
    while (!feof(fp)) {
        size_t n = fread(buf, 1, 512, fp);
        if (n == 0) {
            pone_file_close(world, self);
            pone_throw_str(world, "Cannot read file: %s", strerror(errno));
        }
        pone_str_append_c(world, retval, buf, n);
    }

    return retval;
}

PONE_FUNC(meth_slurp) {
    pone_val* val;
    PONE_ARG("file.slurp", "o", &val);
    if (pone_str_contains_null(world, val)) {
        pone_throw_str(world, "You can't slurp file. Because file name contains \\0.");
    }

    pone_val* str = pone_str_c_str(world, val);
    FILE* fp = fopen(pone_str_ptr(str), "r");
    if (!fp) {
        pone_throw_str(world, "Cannot open '%s': %s", pone_str_ptr(str), strerror(errno));
    }

    pone_val* retval = pone_str_new_strdup(world, "", 0);

    char buf[512];
    while (!feof(fp)) {
        size_t n = fread(buf, 1, 512, fp);
        if (n == 0) {
            fclose(fp);
            pone_throw_str(world, "Cannot read file '%s': %s", pone_str_ptr(str), strerror(errno));
        }
        pone_str_append_c(world, retval, buf, n);
    }

    fclose(fp);

    return retval;
}

void pone_file_init(pone_world* world) {
    // TODO File#printf
    // TODO File#readline
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
    pone_add_method_c(world, klass, "stat", strlen("stat"), meth_file_stat);
    {
        // FileSeqIterator
        pone_val* seq_iter_klass = pone_class_new(world, "FileSeqIterator", strlen("FileSeqIterator"));
        pone_add_method_c(world, seq_iter_klass, "pull-one", sizeof("pull-one") - 1, meth_file_list_iterator_pull_one);

        // Seq class for lines.
        pone_val* seq_klass = pone_class_new(world, "FileLinesSeq", strlen("FileLinesSeq"));
        pone_add_method_c(world, seq_klass, "iterator", sizeof("iterator") - 1, meth_file_seq_iterator);
        pone_obj_set_ivar(world, seq_klass, "FileSeqIterator", seq_iter_klass);

        pone_obj_set_ivar(world, klass, "FileSeq", seq_klass);
        pone_add_method_c(world, klass, "lines", strlen("lines"), meth_file_lines);
    }
    pone_add_method_c(world, klass, "slurp_rest", strlen("slurp_rest"), meth_file_slurp_rest);

    world->universe->class_file = klass;

    pone_val* module = pone_module_new(world, "file");

    pone_module_put(world, module, "File", klass);

    pone_module_put(world, module, "open", pone_code_new_c(world, meth_open));
    pone_module_put(world, module, "fdopen", pone_code_new_c(world, meth_fdopen));

    pone_module_put(world, module, "slurp", pone_code_new_c(world, meth_slurp));

    // TODO dup2

    pone_module_put(world, module, "stdin", pone_file_new(world, stdin, false));
    pone_module_put(world, module, "stdout", pone_file_new(world, stdout, false));
    pone_module_put(world, module, "stderr", pone_file_new(world, stderr, false));

    pone_module_put(world, module, "SEEK_CUR", pone_int_new(world, SEEK_CUR));
    pone_module_put(world, module, "SEEK_SET", pone_int_new(world, SEEK_SET));
    pone_module_put(world, module, "SEEK_END", pone_int_new(world, SEEK_END));
    pone_module_put(world, module, "LOCK_SH", pone_int_new(world, LOCK_SH));
    pone_module_put(world, module, "LOCK_EX", pone_int_new(world, LOCK_EX));
    pone_module_put(world, module, "LOCK_UN", pone_int_new(world, LOCK_UN));

    pone_universe_set_global(world->universe, "file", module);
}
