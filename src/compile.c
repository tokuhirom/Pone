#include "pone.h"
#include "pone_compile.h"

void pone_compile_c(pone_world* world, const char* so_filename, const char* c_filename) {
    pone_val* cmdline = pone_str_c_str(world, pone_str_new_printf(world, "clang -x c -D_POSIX_C_SOURCE=200809L -rdynamic -fPIC -shared -Iinclude/ -I src/ -g -lm -std=c99 -o %s %s -L. -lpone", so_filename, c_filename));
    int retval = system(pone_str_ptr(cmdline));
    if (retval != 0) {
        pone_throw_str(world, "cannot compile generated C code");
    }
}

