#ifndef PONE_H_
#define PONE_H_

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdarg.h>
#include <errno.h>
#include <setjmp.h>
#include <pthread.h>
#include <limits.h>
#include "pone_khash.h"
#include "pone_config.h"

typedef long pone_int_t;
#define PoneIntFmt "%ld"
#define PONE_INT_MAX LONG_MAX

// TODO: NaN boxing

// This variable is global var.
#define PONE_FLAGS_GLOBAL (1 << 0)
// This object is immutable
#define PONE_FLAGS_FROZEN (1 << 1)
// GC mark
#define PONE_FLAGS_GC_MARK (1 << 2)
// type specific flag 1
#define PONE_FLAGS_TYPE_1 (1 << 5)
// type specific flag 2
#define PONE_FLAGS_TYPE_2 (1 << 6)
// type specific flag 3
#define PONE_FLAGS_TYPE_3 (1 << 7)

// string literal is constant
#define PONE_FLAGS_STR_CONST PONE_FLAGS_TYPE_1
// This string has copied buffer
#define PONE_FLAGS_STR_COPY PONE_FLAGS_TYPE_2
// This string is UTF-8 decoded string
#define PONE_FLAGS_STR_UTF8 PONE_FLAGS_TYPE_3

typedef double pone_num_t;

// type flag needs 4 bits
typedef enum {
    PONE_NIL = 1,
    PONE_INT = 2,
    PONE_NUM = 3,
    PONE_STRING = 4,
    PONE_ARRAY = 5,
    PONE_BOOL = 6,
    PONE_MAP = 7,
    PONE_CODE = 8,
    PONE_OBJ = 9,
    PONE_LEX = 10,
    PONE_OPAQUE = 11,
} pone_t;

#define PONE_HEAD \
    uint64_t type : 4, flags : 8;

struct pone_val;
struct pone_world;

typedef struct pone_val* (*pone_funcptr_t)(struct pone_world*, struct pone_val*, int n, va_list);
typedef struct pone_val* (*pone_loadfunc_t)(struct pone_world*, struct pone_val*);
typedef void (*pone_finalizer_t)(struct pone_world*, struct pone_val*);

KHASH_MAP_INIT_STR(str, struct pone_val*)

typedef struct {
    PONE_HEAD;
} pone_nil_t;

typedef struct {
    PONE_HEAD;
} pone_basic;

// integer value
typedef struct {
    PONE_HEAD;
    pone_int_t i;
} pone_int;

typedef struct {
    PONE_HEAD;
    pone_num_t n;
} pone_num;

typedef struct {
    PONE_HEAD;
    union {
        char* p;
        struct pone_val* val;
    };
    pone_int_t len;
} pone_string;

typedef struct {
    PONE_HEAD;
    bool b;
} pone_bool;

typedef struct {
    PONE_HEAD;
    struct pone_val** a;
    pone_int_t max;
    pone_int_t len;
} pone_ary;

struct pone_hash_body;

typedef struct {
    PONE_HEAD;
    struct pone_hash_body* body;
} pone_map;

typedef struct {
    PONE_HEAD;
    struct pone_val* klass;
    // instance variable
    khash_t(str) * ivar;
} pone_obj;

struct pone_opaque_body;

typedef struct {
    PONE_HEAD;
    struct pone_opaque_body* body;
} pone_opaque;

typedef struct pone_lex_t {
    PONE_HEAD;
    struct pone_val* parent;
    khash_t(str) * map;
    pthread_t thread_id;
} pone_lex_t;

#ifndef PONE_ARENA_SIZE
#define PONE_ARENA_SIZE 1024
#endif

struct pone_arena;
struct pone_universe;

struct pone_val_vec {
    pone_int_t n, m;
    struct pone_val** a;
};

// thread context
typedef struct pone_world {
    struct pone_arena* arena_head;
    struct pone_arena* arena_last;

    struct pone_val* code;

    // list of unused values.
    struct pone_val* freelist;

    struct pone_universe* universe;

    // lexical value list
    struct pone_val* lex;

    // $@
    struct pone_val* errvar;

    // $!(errno)
    int errsv;

    // caller stack(it contains pone_ary*)
    struct pone_val* caller_stack;

    // error handler
    jmp_buf* err_handlers;
    struct pone_val** err_handler_lexs;
    pone_int_t* err_handler_callers;
    int err_handler_idx;
    int err_handler_max;

    // save C stack values for saving from GC...
    // tmpstack contains list of values in C stack.
    struct {
        struct pone_val** a;
        size_t n;
        size_t m;
    } tmpstack;

    // pone pushes current position of the tmpstack top.
    struct {
        size_t* a;
        size_t n;
        size_t m;
    } savestack;

    pthread_t thread_id;

    pthread_mutex_t mutex;
    pthread_cond_t cond;

    // True if GC is requested.
    bool gc_requested;

    struct pone_world* next;

    uint64_t random_key[2];
} pone_world;

typedef struct {
    PONE_HEAD;
    pone_funcptr_t func;
    struct pone_val* lex;
} pone_code;

typedef struct pone_val {
    union {
        // list of free'd values.
        // see http://loveruby.net/ja/rhg/book/gc.html
        struct {
            uint64_t type : 4, flags : 8;
            struct pone_val* next; // next free value
        } free;

        pone_basic basic;
        pone_ary ary;
        pone_code code;
        pone_map map;
        pone_string str;
        pone_num num;
        pone_int integer;
        pone_obj obj;
        pone_opaque opaque;
        pone_bool boolean;
        pone_lex_t lex;
    } as;
} pone_val;

#define PONE_SIGNAL_HANDLERS_SIZE 32

// VM context
typedef struct pone_universe {
    // signal handlers
    struct pone_val_vec signal_channels[PONE_SIGNAL_HANDLERS_SIZE];
    // mutex for signal_channels.
    pthread_mutex_t signal_channels_mutex;

    // signal thread.
    pthread_t signal_thread;

    pone_world* global_world;

    pone_world* signal_world;

    // True after pone_universe() was called.
    bool end_time;

    // class of class
    struct pone_val* class_class;
    // class of Int
    struct pone_val* class_int;
    // class of Num
    struct pone_val* class_num;
    // class of Str
    struct pone_val* class_str;
    // class of Bytes
    struct pone_val* class_bytes;
    // class of Array
    struct pone_val* class_ary;
    // class of Bool
    struct pone_val* class_bool;
    // class of Map
    struct pone_val* class_map;
    // class of Code
    struct pone_val* class_code;
    // class of Nil
    struct pone_val* class_nil;
    // class of Range
    struct pone_val* class_range;
    // We use a sentinel value to mark the end of an iteration.
    // This is "IterationEnd" in rakudo Perl6.
    struct pone_val* instance_iteration_end;
    // class of Regex
    struct pone_val* class_regex;
    // class of onig(private)
    struct pone_val* class_onig;
    // class of Match
    struct pone_val* class_match;
    // class of Pair
    struct pone_val* class_pair;
    // class of Channel
    struct pone_val* class_channel;
    // class of Errno
    struct pone_val* class_errno;
    // class of File
    struct pone_val* class_file;
    // class of Module
    struct pone_val* class_module;
    // class of TmpFile
    struct pone_val* class_tmpfile;
    // class of X::AdHoc
    struct pone_val* class_x_adhoc;
    // class of Path
    struct pone_val* class_path;
    // class of Stat
    struct pone_val* class_stat;
    // class of Time
    struct pone_val* class_time;
    // class of Caller
    struct pone_val* class_caller;

    // $*INC
    struct pone_val* inc;
    // $*ARGS
    struct pone_val* args;

    khash_t(str) * globals;

    // thread.c sends signal at thread termination.
    pthread_cond_t worker_fin_cond;

    // list of normal worlds.
    // This list contains worlds created by pone_thread_start.
    // This doesn't include system worlds.
    pone_world* worker_worlds;

    // compiler world.
    pone_world* compiler_world;

    // mutex for above
    pthread_mutex_t worker_worlds_mutex;

    FILE* gc_log;
} pone_universe;

typedef struct pone_arena {
    struct pone_arena* next;
    int idx;
    struct pone_val values[PONE_ARENA_SIZE];
} pone_arena;

// pone.c
void pone_init(pone_universe* universe);

// nil.c
pone_val* pone_nil();
void pone_nil_init(pone_world* world);

// world.c
pone_world* pone_world_new(pone_universe* universe);
void pone_world_free(pone_world* world);
void pone_world_release(pone_world* world);
pone_val* pone_try(pone_world* world, pone_val* code);
pone_val* pone_errvar(pone_world* world);
pone_val* pone_errno(pone_world* world);
void pone_world_mark(pone_world*);
void pone_world_set_errno(pone_world* world);
void pone_use(pone_world* world, const char* pkg, const char* as, pone_int_t as_len);

// op.c
void pone_dd(pone_world* world, pone_val* val);
const char* pone_what_str_c(pone_world* world, pone_val* val);
const char* pone_type_name(pone_val* val);
pone_val* pone_bitwise_or(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_bitwise_and(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_bitwise_xor(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_brshift(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_blshift(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_inplace_add(pone_world* world, int level, const char* varname, pone_val* val);
pone_val* pone_inplace_sub(pone_world* world, int level, const char* varname, pone_val* val);
pone_val* pone_inplace_mul(pone_world* world, int level, const char* varname, pone_val* val);
pone_val* pone_inplace_mod(pone_world* world, int level, const char* varname, pone_val* val);
pone_val* pone_inplace_div(pone_world* world, int level, const char* varname, pone_val* val);
pone_val* pone_inplace_pow(pone_world* world, int level, const char* varname, pone_val* val);
pone_val* pone_inplace_bin_or(pone_world* world, int level, const char* varname, pone_val* val);
pone_val* pone_inplace_bin_and(pone_world* world, int level, const char* varname, pone_val* val);
pone_val* pone_inplace_bin_xor(pone_world* world, int level, const char* varname, pone_val* val);
pone_val* pone_inplace_blshift(pone_world* world, int level, const char* varname, pone_val* val);
pone_val* pone_inplace_brshift(pone_world* world, int level, const char* varname, pone_val* val);
pone_val* pone_inplace_concat_s(pone_world* world, int level, const char* varname, pone_val* val);
pone_int_t pone_str_cmp(pone_world* world, pone_val* v1, pone_val* v2);

// hash.c
pone_val* pone_map_new(pone_world* world);
pone_val* pone_map_assign_keys(pone_world* world, pone_val* hash, pone_int_t n, ...);
void pone_map_assign_key(pone_world* world, pone_val* hv, pone_val* k, pone_val* v);
pone_int_t pone_map_size(pone_val* val);
void pone_map_free(pone_world* world, pone_val* val);
pone_val* pone_map_at_key(pone_world* world, pone_val* hash, pone_val* key);
void pone_map_init(pone_world* world);
pone_val* pone_map_keys(pone_world* world, pone_val* val);
void pone_map_mark(pone_val* val);
pone_val* pone_map_copy(pone_world* world, pone_val* obj);

// array.c
pone_val* pone_ary_new(pone_world* world, pone_int_t n, ...);
pone_int_t pone_ary_size(pone_val* val);
void pone_ary_free(pone_world* world, pone_val* val);
void pone_ary_init(pone_world* world);
pone_val* pone_ary_at_pos(pone_val* ary, pone_int_t n);
void pone_ary_push(pone_world* world, pone_val* self, pone_val* val);
void pone_ary_assign_pos(pone_world* world, pone_val* self, pone_val* pos, pone_val* val);
void pone_ary_mark(pone_val* val);
pone_val* pone_ary_pop(pone_world* world, pone_val* self);
pone_val* pone_ary_last(pone_world* world, pone_val* self);
pone_val* pone_ary_shift(pone_world* world, pone_val* self);
pone_val* pone_ary_copy(pone_world* world, pone_val* obj);

// str.c
inline pone_int_t pone_str_next_char_len(char* s);
pone_val* pone_bytes_new_const(pone_world* world, const char* p, size_t len);
pone_val* pone_bytes_new_allocd(pone_world* world, char* p, size_t len);
pone_val* pone_str_new_strdup(pone_world* world, const char* p, size_t len);
pone_val* pone_bytes_new_malloc(pone_world* world, pone_int_t len);
pone_val* pone_bytes_new_strdup(pone_world* world, const char* p, size_t len);
void pone_bytes_truncate(pone_val* val, pone_int_t len);
pone_val* pone_str_new_allocd(pone_world* world, char* p, size_t len);
pone_val* pone_str_new_const(pone_world* world, const char* p, size_t len);
void pone_str_free(pone_world* world, pone_val* val);
pone_val* pone_stringify(pone_world* world, pone_val* val);
pone_val* pone_str_from_num(pone_world* world, double n);
char* pone_str_ptr(pone_val* val);
pone_int_t pone_str_len(pone_val* val);
char* pone_strdup(pone_world* world, const char* src, size_t size);
void pone_str_append_c(pone_world* world, pone_val* str, const char* s, pone_int_t s_len);
void pone_str_append(pone_world* world, pone_val* str, pone_val* s);
void pone_str_appendf(pone_world* world, pone_val* str, const char* fmt, ...);
void pone_str_init(pone_world* world);
pone_val* pone_str_new_vprintf(pone_world* world, const char* fmt, va_list args);
pone_val* pone_str_new_printf(pone_world* world, const char* fmt, ...);
bool pone_str_contains_null(pone_world* world, pone_val* val);
pone_val* pone_str_c_str(pone_world* world, pone_val* val);
pone_val* pone_str_copy(pone_world* world, pone_val* val);
void pone_str_mark(pone_val* val);

// code.c
pone_val* pone_code_new_c(pone_world* world, pone_funcptr_t func);
void pone_code_bind(pone_world* world, pone_val* code, const char* key, pone_val* val);
pone_val* pone_code_new(pone_world* world, pone_funcptr_t func);
pone_val* pone_code_call(pone_world* world, const char* filename, int lineno, const char* sub, pone_val* code, pone_val* self, int n, ...);
pone_val* pone_code_vcall(pone_world* world, const char* filename, int lineno, const char* sub, pone_val* code, pone_val* self, int n, va_list args);
void pone_code_init(pone_world* world);
void pone_code_mark(pone_val* val);

// int.c
pone_val* pone_str_from_int(pone_world* world, pone_int_t i);
pone_int_t pone_int_val(pone_val* val);
pone_val* pone_int_incr(pone_world* world, pone_val* i);
pone_val* pone_int_new(pone_world* world, pone_int_t i);
void pone_int_init(pone_world* world);

// SV ops
double pone_num_val(pone_val* val);
bool pone_bool_val(pone_val* val);
pone_int_t pone_intify(pone_world* world, pone_val* val);
pone_num_t pone_numify(pone_world* world, pone_val* val);
bool pone_is_frozen(pone_val* v);

// scope.c
void pone_push_scope(pone_world* world);
void pone_pop_scope(pone_world* world);
pone_val* pone_lex_new(pone_world* world, pone_val* parent);
void pone_lex_free(pone_world* world, pone_val* lex);
void pone_lex_mark(pone_val* lex);
void pone_lex_dump(pone_val* lex);
pone_val* pone_save_tmp(pone_world* world, pone_val* val);

// universe.c
pone_universe* pone_universe_init();
void pone_universe_destroy(pone_universe* universe);
void pone_universe_default_err_handler(pone_world* world);
void pone_universe_set_global(pone_universe* universe, const char* key, pone_val* val);
void pone_universe_mark(pone_universe*);
void pone_universe_wait_threads(pone_universe* universe);
void pone_gc_log(pone_universe* unvierse, const char* fmt, ...);

// bool.c
pone_val* pone_true();
pone_val* pone_false();
void pone_bool_init(pone_world* world);

// num.c
pone_val* pone_num_new(pone_world* world, pone_num_t i);
void pone_num_init(pone_world* world);

// basic value operations
static inline pone_t pone_type(pone_val* val) {
    return val->as.basic.type;
}
static inline pone_t pone_flags(pone_val* val) {
    return val->as.basic.flags;
}
static inline bool pone_defined(pone_val* val) {
    return val->as.basic.type != PONE_NIL;
}
static inline bool pone_alive(pone_val* val) {
    return val->as.basic.type != 0;
}

// op.c
pone_val* pone_get_lex(pone_world* world, const char* key);
pone_val* pone_assign(pone_world* world, int up, const char* key, pone_val* val);
pone_val* pone_assign_list(pone_world* world, pone_val* val, int nvars, ...);
pone_val* pone_assign_pos(pone_world* world, pone_val* var, pone_val* pos, pone_val* rhs);
pone_val* pone_add(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_subtract(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_multiply(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_divide(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_mod(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_pow(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_str_concat(pone_world* world, pone_val* v1, pone_val* v2);
bool pone_so(pone_val* val);
pone_val* pone_smart_match(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_assign_key(pone_world* world, pone_val* self, pone_val* key, pone_val* val);
pone_val* pone_not(pone_world* world, pone_val* val);

// iter.c
pone_val* pone_iter_init(pone_world* world, pone_val* val);
pone_val* pone_iter_next(pone_world* world, pone_val* iter);

void pone_val_free(pone_world* world, pone_val* p);
pone_t pone_type(pone_val* val);
void* pone_malloc(pone_world* world, size_t size);
pone_val* pone_obj_alloc(pone_world* world, pone_t type);
void pone_free(pone_world* world, void* p);

// class.c
pone_val* pone_init_class(pone_world* world);
pone_val* pone_class_new(pone_world* world, const char* name, size_t name_len);
void pone_add_method(pone_world* world, pone_val* klass, const char* name, size_t name_len, pone_val* method);
void pone_add_method_c(pone_world* world, pone_val* klass, const char* name, size_t name_len, pone_funcptr_t funcptr);
pone_val* pone_find_method(pone_world* world, pone_val* klass, const char* name);
pone_val* pone_what(pone_world* world, pone_val* obj);
pone_val* pone_call_method(pone_world* world, const char* filename, int lineno, const char* subname, pone_val* obj, const char* method_name, int n, ...);
pone_val* pone_call_meta_method(pone_world* world, pone_val* obj, const char* method_name, int n, ...);

// obj.c
pone_val* pone_init_mu(pone_world* world);
pone_val* pone_obj_new(pone_world* world, pone_val* klass);
void pone_obj_free(pone_world* world, pone_val* val);
void pone_obj_set_ivar(pone_world* world, pone_val* obj, const char* name, pone_val* val);
pone_val* pone_obj_get_ivar(pone_world* world, pone_val* obj, const char* name);
void pone_obj_mark(pone_val* val);
pone_val* pone_val_copy(pone_world* world, pone_val* obj);

// op.c
pone_val* pone_at_pos(pone_world* world, pone_val* obj, pone_val* pos);
pone_val* pone_at_key(pone_world* world, pone_val* obj, pone_val* key);
bool pone_eq(pone_world* world, pone_val* v1, pone_val* v2);
bool pone_ne(pone_world* world, pone_val* v1, pone_val* v2);
bool pone_le(pone_world* world, pone_val* v1, pone_val* v2);
bool pone_lt(pone_world* world, pone_val* v1, pone_val* v2);
bool pone_ge(pone_world* world, pone_val* v1, pone_val* v2);
bool pone_gt(pone_world* world, pone_val* v1, pone_val* v2);
bool pone_str_eq(pone_world* world, pone_val* v1, pone_val* v2);
bool pone_str_ne(pone_world* world, pone_val* v1, pone_val* v2);
bool pone_str_le(pone_world* world, pone_val* v1, pone_val* v2);
bool pone_str_lt(pone_world* world, pone_val* v1, pone_val* v2);
bool pone_str_ge(pone_world* world, pone_val* v1, pone_val* v2);
bool pone_str_gt(pone_world* world, pone_val* v1, pone_val* v2);

// range.c
pone_val* pone_range_new(pone_world* world, pone_val* min, pone_val* max);
void pone_range_init(pone_world* world);

// re.c
pone_val* pone_regex_new(pone_world* world, const char* str, size_t len);
void pone_regex_init(pone_world* world);
pone_val* pone_match_new(pone_world* world, pone_val* orig, pone_int_t from, pone_int_t to);
void pone_match_push(pone_world* world, pone_val* self, pone_int_t from, pone_int_t to);

// thread.c
void pone_thread_init(pone_world* world);
void pone_thread_start(pone_universe* universe, pone_val* code);

// pair.c
void pone_pair_init(pone_world* world);
pone_val* pone_pair_new(pone_world* world, pone_val* key, pone_val* value);

// gc.c
void pone_gc_mark_value(pone_val* val);
void pone_gc_init(pone_world* world);
void pone_gc_run(pone_world* world);

// signal.c
void pone_signal_notify(pone_world* world, pone_int_t sig, pone_val* code);
void pone_signal_start_thread(pone_world* world);
void pone_signal_init(pone_world* world);

// channel.c
pone_val* pone_chan_new(pone_world* world, pone_int_t limit);
bool pone_chan_trysend(pone_world* world, pone_val* chan, pone_val* val);

// errno.c
pone_val* pone_errno(pone_world* world);
void pone_errno_init(pone_world* world);

// builtin.c
void pone_builtin_init(pone_world* world);

#ifdef DEBUG_THREAD
#define THREAD_TRACE(fmt, ...) fprintf(stderr, "[pone thread] [%lx] " fmt "\n", pthread_self(), ##__VA_ARGS__)
#else
#define THREAD_TRACE(fmt, ...)
#endif

#ifdef DEBUG_MODULE
#define MODULE_TRACE(fmt, ...) fprintf(stderr, "[pone module] [%lx] " fmt "\n", pthread_self(), ##__VA_ARGS__)
#else
#define MODULE_TRACE(fmt, ...)
#endif

#ifdef DEBUG_GC
#define GC_TRACE(fmt, ...) fprintf(stderr, "[pone gc] [%lx] " fmt "\n", pthread_self(), ##__VA_ARGS__)
#else
#define GC_TRACE(fmt, ...)
#endif

#ifdef DEBUG_EXC
#define EXC_TRACE(fmt, ...) fprintf(stderr, "[pone exc] " fmt "\n", ##__VA_ARGS__)
#else
#define EXC_TRACE(fmt, ...)
#endif

#ifdef DEBUG_WORLD
#define WORLD_TRACE(fmt, ...) fprintf(stderr, "[pone world] " fmt "\n", ##__VA_ARGS__)
#else
#define WORLD_TRACE(fmt, ...)
#endif

#ifdef GC_RD_LOCK_DEBUG
#define GC_RD_LOCK_TRACE(fmt, ...) fprintf(stderr, "[pone gc rd lock] " fmt "\n", ##__VA_ARGS__)
#else
#define GC_RD_LOCK_TRACE(fmt, ...)
#endif

#define PONE_ALLOC_CHECK(v)                              \
    do {                                                 \
        if (!v) {                                        \
            fprintf(stderr, "Cannot allocate memory\n"); \
            abort();                                     \
        }                                                \
    } while (0)

#ifdef __linux__
#include <unistd.h>
#include <sys/syscall.h>
#define ASSERT_LOCK(lock) assert((lock).__data.__owner == syscall(SYS_gettid))
#else
#define ASSERT_LOCK(lock)
#endif

#define CHECK_PTHREAD(code)                                     \
    do {                                                        \
        int r;                                                  \
        THREAD_TRACE("%s %s at %d", #code, __FILE__, __LINE__); \
        if ((r = (code)) != 0) {                                \
            errno = r;                                          \
            perror("pthread error: " #code);                    \
            abort();                                            \
        }                                                       \
    } while (0)

void pone_runtime_init(pone_world* world);

#define PONE_FUNC(name) static pone_val* name(pone_world* world, pone_val* self, int nargs, va_list args)
#define PONE_ARG(name, spec, ...) pone_arg(world, name, nargs, args, spec, ##__VA_ARGS__)
void pone_arg(pone_world* world, const char* name, int nargs, va_list args, const char* spec, ...);

#define PONE_REG_METHOD(name, meth) \
    pone_add_method_c(world, klass, name, strlen(name), meth);

#define PONE_DECLARE_GETTER(name, var)                                              \
    static pone_val* name(pone_world* world, pone_val* self, int n, va_list args) { \
        assert(n == 0);                                                             \
        pone_val* v = pone_obj_get_ivar(world, self, var);                          \
        return v;                                                                   \
    }

#endif
