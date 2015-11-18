#ifndef PONE_H_

#define PONE_H_

#ifndef _POSIX_SOURCE
#define _POSIX_SOURCE
#endif

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
#include "khash.h"
#include "pone_config.h"

typedef long pone_int_t;
#define PoneIntFmt "%ld"

// TODO: NaN boxing

// This variable is global var.
#define PONE_FLAGS_GLOBAL (1<<0)
// This object is immutable
#define PONE_FLAGS_FROZEN (1<<1)
// GC mark
#define PONE_FLAGS_GC_MARK (1<<2)
// type specific flag 1
#define PONE_FLAGS_TYPE_1 (1<<5)
// type specific flag 2
#define PONE_FLAGS_TYPE_2 (1<<6)
// type specific flag 3
#define PONE_FLAGS_TYPE_3 (1<<7)

// string literal is constant
#define PONE_FLAGS_STR_CONST  PONE_FLAGS_TYPE_1
// This string has copied buffer
#define PONE_FLAGS_STR_COPY   PONE_FLAGS_TYPE_2

typedef double pone_num_t;

typedef enum {
    PONE_NIL=1,
    PONE_INT,
    PONE_NUM,
    PONE_STRING,
    PONE_ARRAY,
    PONE_BOOL,
    PONE_HASH,
    PONE_CODE,
    PONE_OBJ,
    PONE_LEX,
    PONE_OPAQUE,
} pone_t;

#define PONE_HEAD \
    pone_t type; \
    uint8_t flags

struct pone_val;
struct pone_world;

typedef struct pone_val* (*pone_funcptr_t)(struct pone_world*, struct pone_val*, int n, va_list);
typedef void(*pone_finalizer_t)(struct pone_world*, struct pone_val*);

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

typedef struct {
    PONE_HEAD;
    khash_t(str) *h;
    pone_int_t len;
} pone_hash;

typedef struct {
    PONE_HEAD;
    struct pone_val* klass;
    // instance variable
    khash_t(str) *ivar;
} pone_obj;

typedef struct {
    PONE_HEAD;
    struct pone_val* klass;
    void *ptr;
    pone_finalizer_t finalizer;
} pone_opaque;

typedef struct pone_lex_t {
    PONE_HEAD;
    struct pone_val* parent;
    khash_t(str) *map;
    pthread_t thread_id;
} pone_lex_t;

#ifndef PONE_ARENA_SIZE
#define PONE_ARENA_SIZE 1024
#endif

struct pone_arena;
struct pone_universe;

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

    // error handler
    jmp_buf* err_handlers;
    struct pone_val** err_handler_lexs;
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
            uint64_t flags;
            struct pone_val* next; // next free value
        } free;

        pone_basic basic;
        pone_ary ary;
        pone_code code;
        pone_hash hash;
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

typedef struct { pone_int_t n, m; pone_val **a; } pone_val_vec;

// VM context
typedef struct pone_universe {
    // signal handlers
    pone_val_vec signal_channels[PONE_SIGNAL_HANDLERS_SIZE];

    // 無("Mu")
    struct pone_val* class_mu;
    // class of class
    struct pone_val* class_class;
    // class of Int
    struct pone_val* class_int;
    // class of Num
    struct pone_val* class_num;
    // class of Str
    struct pone_val* class_str;
    // class of Cool
    struct pone_val* class_cool;
    // class of Any
    struct pone_val* class_any;
    // class of Array
    struct pone_val* class_ary;
    // class of Bool
    struct pone_val* class_bool;
    // class of Hash
    struct pone_val* class_hash;
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
    // class of Match
    struct pone_val* class_match;
    // class of Thread
    struct pone_val* class_thread;
    // class of Pair
    struct pone_val* class_pair;
    // class of IO::Socket::INET
    struct pone_val* class_io_socket_inet;
    // class of Channel
    struct pone_val* class_channel;
    // class of Opaque
    struct pone_val* class_opaque;
    // class of Opaque
    struct pone_val* class_errno;

    khash_t(str) *globals;

    // thread.c sends signal at thread termination.
    pthread_cond_t thread_terminate_cond;

    // UNIVERSE lock. You need to lock this before modify this object.
    pthread_mutex_t universe_mutex;

    // list of normal worlds.
    // This list contains worlds created by pone_thread_start.
    // This doesn't include system worlds.
    pone_world* normal_worlds;

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

// exc.c
jmp_buf* pone_exc_handler_push(pone_world* world);
void pone_exc_handler_pop(pone_world* world);
void pone_throw(pone_world* world, pone_val* msg);
void pone_throw_str(pone_world* world, const char* fmt, ...);
void pone_warn_str(pone_world* world, const char* fmt, ...);

// op.c
void pone_dd(pone_world* world, pone_val* val);
const char* pone_what_str_c(pone_val* val);

// hash.c
pone_val* pone_hash_new(pone_world* world);
pone_val* pone_hash_assign_keys(pone_world* world, pone_val* hash, pone_int_t n, ...);
void pone_hash_assign_key_c(pone_world* world, pone_val* hv, const char* key, pone_int_t key_len, pone_val* v);
void pone_hash_assign_key(pone_world* world, pone_val* hv, pone_val* k, pone_val* v);
size_t pone_hash_elems(pone_val* val);
void pone_hash_free(pone_world* world, pone_val* val);
pone_val* pone_hash_at_key_c(pone_universe* universe, pone_val* hash, const char* name);
void pone_hash_init(pone_world* world);
bool pone_hash_exists_c(pone_world* world, pone_val* hash, const char* name);
pone_val* pone_hash_keys(pone_world* world, pone_val* val);
void pone_hash_mark(pone_val* val);

// array.c
pone_val* pone_ary_new(pone_world* world, pone_int_t n, ...);
pone_int_t pone_ary_elems(pone_val* val);
void pone_ary_free(pone_world* world, pone_val* val);
void pone_ary_init(pone_world* world);
pone_val* pone_ary_at_pos(pone_val* ary, pone_int_t n);
void pone_ary_push(pone_universe* universe, pone_val* self, pone_val* val);
void pone_ary_assign_pos(pone_world* world, pone_val* self, pone_val* pos, pone_val* val);
void pone_ary_mark(pone_val* val);
pone_val* pone_ary_pop(pone_world* world, pone_val* self);
pone_val* pone_ary_last(pone_world* world, pone_val* self);
pone_val* pone_ary_shift(pone_world* world, pone_val* self);

// str.c
pone_val* pone_str_new(pone_world* world, const char*p, size_t len);
pone_val* pone_str_new_const(pone_world* world, const char*p, size_t len);
void pone_str_free(pone_world* world, pone_val* val);
pone_val* pone_stringify(pone_world* world, pone_val* val);
pone_val* pone_str_from_num(pone_world* world, double n);
const char* pone_str_ptr(pone_val* val);
pone_int_t pone_str_len(pone_val* val);
char* pone_strdup(pone_world* world, const char* src, size_t size);
void pone_str_append_c(pone_world* world, pone_val* str, const char* s, pone_int_t s_len);
void pone_str_append(pone_world* world, pone_val* str, pone_val* s);
void pone_str_appendf(pone_world* world, pone_val* str, const char* fmt, ...);
void pone_str_init(pone_world* world);
pone_val* pone_str_new_vprintf(pone_world* world, const char* fmt, va_list args);
pone_val* pone_str_new_printf(pone_world* world, const char* fmt, ...);
bool pone_str_contains_null(pone_universe* universe, pone_val* val);
pone_val* pone_str_c_str(pone_world* world, pone_val* val);
pone_val* pone_str_copy(pone_world* world, pone_val* val);
void pone_str_mark(pone_val* val);

// code.c
pone_val* pone_code_new_c(pone_world* world, pone_funcptr_t func);
pone_val* pone_code_new(pone_world* world, pone_funcptr_t func);
pone_val* pone_code_call(pone_world* world, pone_val* code, pone_val* self, int n, ...);
pone_val* pone_code_vcall(pone_world* world, pone_val* code, pone_val* self, int n, va_list args);
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
size_t pone_elems(pone_world* world, pone_val* val);
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
static inline pone_t pone_type(pone_val* val) { return val->as.basic.type; }
static inline pone_t pone_flags(pone_val* val) { return val->as.basic.flags; }
static inline bool pone_defined(pone_val* val) { return val->as.basic.type != PONE_NIL; }
static inline bool pone_alive(pone_val* val) { return val->as.basic.type != 0; }

// op.c
pone_val* pone_get_lex(pone_world* world, const char* key);
pone_val* pone_assign(pone_world* world, int up, const char* key, pone_val* val);
pone_val* pone_assign_pos(pone_world* world, pone_val* var, pone_val* pos, pone_val* rhs);
pone_val* pone_add(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_subtract(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_multiply(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_divide(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_mod(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_str_concat(pone_world* world, pone_val* v1, pone_val* v2);
bool pone_so(pone_val* val);
pone_val* pone_smart_match(pone_world* world, pone_val* v1, pone_val* v2);
pone_val* pone_assign_key(pone_world* world, pone_val* self, pone_val* key, pone_val* val);

// iter.c
pone_val* pone_iter_init(pone_world* world, pone_val* val);
pone_val* pone_iter_next(pone_world* world, pone_val* iter);

// builtin.c
pone_val* pone_builtin_slurp(pone_world* world, pone_val* val);
pone_val* pone_builtin_dd(pone_world* world, pone_val* val);
pone_val* pone_builtin_abs(pone_world* world, pone_val* val);
pone_val* pone_builtin_print(pone_world* world, pone_val* val);
pone_val* pone_builtin_say(pone_world* world, pone_val* val);
pone_val* pone_builtin_elems(pone_world* world, pone_val* val);
pone_val* pone_builtin_time(pone_world* world);
pone_val* pone_builtin_getenv(pone_world* world, pone_val* key);
pone_val* pone_builtin_sleep(pone_world* world, pone_val* vi);
pone_val* pone_builtin_signal(pone_world* world, pone_val* sig_val, pone_val* code);
pone_val* pone_builtin_die(pone_world* world, pone_val* msg);
pone_val* pone_builtin_printf(pone_world* world, pone_val* fmt, ...);
pone_val* pone_builtin_chan(pone_world* world, pone_val* limit);
pone_val* pone_builtin_pthread_self(pone_world* world);
pone_val* pone_builtin_exit(pone_world* world);

void pone_val_free(pone_world* world, pone_val* p);
pone_t pone_type(pone_val* val);
void* pone_malloc(pone_universe* universe, size_t size);
pone_val* pone_obj_alloc(pone_world* world, pone_t type);
void pone_free(pone_universe* universe, void* p);

// cool.c
pone_val* pone_init_cool(pone_world* world);

// any.c
pone_val* pone_init_any(pone_world* world);

// class.c
pone_val* pone_init_class(pone_world* world);
pone_val* pone_class_new(pone_world* world, const char* name, size_t name_len);
void pone_class_push_parent(pone_world* world, pone_val* obj, pone_val* klass);
void pone_add_method(pone_world* world, pone_val* klass, const char* name, size_t name_len, pone_val* method);
void pone_add_method_c(pone_world* world, pone_val* klass, const char* name, size_t name_len, pone_funcptr_t funcptr);
pone_val* pone_find_method(pone_world* world, pone_val* klass, const char* name);
pone_val* pone_what(pone_world* world, pone_val* obj);
pone_val* pone_call_method(pone_world* world, pone_val* obj, const char* method_name, int n, ...);
pone_val* pone_call_meta_method(pone_world* world, pone_val* obj, const char* method_name, int n, ...);
void pone_class_compose(pone_world* world, pone_val* klass);

// obj.c
pone_val* pone_init_mu(pone_world* world);
pone_val* pone_obj_new(pone_world* world, pone_val* klass);
void pone_obj_free(pone_world* world, pone_val* val);
void pone_obj_set_ivar(pone_world* world, pone_val* obj, const char* name, pone_val* val);
pone_val* pone_obj_get_ivar(pone_world* world, pone_val* obj, const char* name);
void pone_obj_mark(pone_val* val);

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

// socket.c
void pone_sock_init(pone_world* world);

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
void pone_channel_init(pone_world* world);
pone_val* pone_chan_new(pone_world* world, pone_int_t limit);
bool pone_chan_trysend(pone_world* world, pone_val* chan, pone_val* val);

// opaque.c
void pone_opaque_init(pone_world* world);
pone_val* pone_opaque_new(pone_world* world, void* ptr, pone_finalizer_t finalizer);
static inline void pone_opaque_set_class(pone_world* world, pone_val* v, pone_val* klass) {
    v->as.opaque.klass = klass;
}
void pone_opaque_free(pone_world* world, pone_val* v);
static inline void* pone_opaque_ptr(pone_val* v) { return v->as.opaque.ptr; }

// errno.c
pone_val* pone_errno(pone_world* world);
void pone_errno_init(pone_world* world);

#ifdef DEBUG_THREAD
#define THREAD_TRACE(fmt, ...) fprintf(stderr, "[pone thread] [%lx] " fmt "\n", pthread_self(), ##__VA_ARGS__)
#else
#define THREAD_TRACE(fmt, ...)
#endif

#ifdef DEBUG_GC
#define GC_TRACE(fmt, ...) fprintf(stderr, "[pone gc] " fmt "\n", ##__VA_ARGS__)
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

#define UNIVERSE_LOCK(universe) \
  do { \
      THREAD_TRACE("UNIVERSE LOCK: thread:%lx(%s line %d)", pthread_self(), __func__, __LINE__); \
      pthread_mutex_lock(&((universe)->universe_mutex)); \
  } while (0)
#define UNIVERSE_UNLOCK(universe) \
  do { \
      THREAD_TRACE("UNIVERSE UNLOCK: thread:%lx", pthread_self()); \
      pthread_mutex_unlock(&((universe)->universe_mutex)); \
  } while(0)

#define PONE_ALLOC_CHECK(v) \
  do { \
    if (!v) { \
      fprintf(stderr, "Cannot allocate memory\n"); \
      abort(); \
    } \
  } while (0)

#ifdef __linux__
#include <unistd.h>
#include <sys/syscall.h>
#define ASSERT_LOCK(lock) assert((lock).__data.__owner==syscall(SYS_gettid))
#else
#define ASSERT_LOCK(lock)
#endif

#define ASSERT_UNIVERSE_LOCK(universe) ASSERT_LOCK((universe)->universe_mutex)

#define CHECK_PTHREAD(code) \
  do { \
      int r; \
      THREAD_TRACE("%s", #code); \
      if ((r=(code)) != 0) { \
          errno = r; \
          perror("pthread error: " #code); \
          abort(); \
      } \
  } while (0)

#define PONE_FUNC(name) static pone_val* name(pone_world* world, pone_val* self, int n, va_list args)

#define PONE_DECLARE_GETTER(name, var) \
    static pone_val* name(pone_world* world, pone_val* self, int n, va_list args) { \
        assert(n == 0); \
        pone_val* v = pone_obj_get_ivar(world, self, var); \
        return v; \
    }

#endif

