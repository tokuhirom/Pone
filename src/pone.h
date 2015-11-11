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
#include <setjmp.h>
#include <pthread.h>
#include "khash.h" /* PONE_INC */
#include "rockre.h"

typedef long pone_int_t;
#define PoneIntFmt "%ld"

// TODO: NaN boxing

// This variable is global var.
#define PONE_FLAGS_GLOBAL (1<<0)
// This object is immutable
#define PONE_FLAGS_FROZEN (1<<1)
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
    PONE_OBJ
} pone_t;

#define PONE_HEAD \
    pone_t type; \
    int refcnt; \
    uint8_t flags

struct pone_val;

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

typedef struct pone_lex_t {
    PONE_HEAD;
    struct pone_lex_t* parent;
    khash_t(str) *map;
} pone_lex_t;

#ifndef PONE_ARENA_SIZE
#define PONE_ARENA_SIZE 1024
#endif

struct pone_arena;
struct pone_universe;

// Calling context
typedef struct pone_world {
    int refcnt;

    struct pone_universe* universe;

    // save last tmpstack_floor
    size_t* savestack;
    size_t savestack_idx;
    size_t savestack_max;

    // mortals we've made
    struct pone_val** tmpstack;
    size_t tmpstack_idx;
    size_t tmpstack_floor;
    size_t tmpstack_max;

    // lexical value list
    pone_lex_t* lex;

    // root lex entry in this world
    pone_lex_t* orig_lex;

    // parent context
    struct pone_world* parent;
    // linked-list for gc
    struct pone_world* next;
    struct pone_world* prev;
} pone_world;

typedef struct pone_val* (*pone_funcptr_t)(pone_world*, struct pone_val*, int n, va_list);

typedef struct {
    PONE_HEAD;
    pone_funcptr_t func;
    pone_lex_t* lex;
} pone_code;

typedef struct pone_val {
    union {
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
        pone_bool boolean;
    } as;
} pone_val;

typedef struct pone_thread_t {
    struct pone_thread_t * next;
    pthread_t thread;
} pone_thread_t;

// VM context
typedef struct pone_universe {
    struct pone_arena* arena_head;
    struct pone_arena* arena_last;

    // list of unused values.
    struct pone_val* freelist;

    // signal handlers
    struct pone_val *signal_handlers[32];

    // $!($@ in perl5)
    struct pone_val* errvar;

    jmp_buf* err_handlers;
    pone_world** err_handler_worlds;
    int err_handler_idx;
    int err_handler_max;

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

    khash_t(str) *globals;

    rockre* rockre;

    // global interpreter lock
    pthread_mutex_t mutex;

    pone_thread_t* threads;
    int thread_num;

    // list of world for gc
    pone_world* world_head;
} pone_universe;

typedef struct pone_arena {
    struct pone_arena* next;
    int idx;
    struct pone_val values[PONE_ARENA_SIZE];
} pone_arena;


// pone.c
void pone_init();

// nil.c
pone_val* pone_nil();
void pone_nil_init(pone_universe* universe);

// world.c
pone_world* pone_world_new(pone_universe* universe);
pone_world* pone_world_new_from_world(pone_world* world, pone_lex_t* lex);
void pone_world_refcnt_inc(pone_world* world);
void pone_world_refcnt_dec(pone_world* world);
pone_val* pone_try(pone_world* world, pone_val* code);
pone_val* pone_errvar(pone_world* world);


// exc.c
jmp_buf* pone_exc_handler_push(pone_world* world);
void pone_exc_handler_pop(pone_world* world);
void pone_throw(pone_world* world, pone_val* msg);
void pone_throw_str(pone_world* world, const char* fmt, ...);
void pone_warn_str(pone_world* world, const char* fmt, ...);

// op.c
void pone_dd(pone_universe* universe, pone_val* val);
const char* pone_what_str_c(pone_val* val);

// hash.c
pone_val* pone_hash_new(pone_universe* universe);
pone_val* pone_hash_assign_keys(pone_world* world, pone_val* hash, pone_int_t n, ...);
void pone_hash_assign_key_c(pone_universe* universe, pone_val* hv, const char* key, pone_int_t key_len, pone_val* v);
void pone_hash_assign_key(pone_world* world, pone_val* hv, pone_val* k, pone_val* v);
size_t pone_hash_elems(pone_val* val);
void pone_hash_free(pone_universe* universe, pone_val* val);
pone_val* pone_hash_at_key_c(pone_universe* universe, pone_val* hash, const char* name);
void pone_hash_init(pone_universe* universe);
bool pone_hash_exists_c(pone_universe* universe, pone_val* hash, const char* name);
pone_val* pone_hash_keys(pone_world* world, pone_val* val);

// array.c
pone_val* pone_ary_new(pone_universe* universe, pone_int_t n, ...);
pone_int_t pone_ary_elems(pone_val* val);
void pone_ary_free(pone_universe* universe, pone_val* val);
void pone_ary_init(pone_universe* universe);
pone_val* pone_ary_at_pos(pone_val* ary, pone_int_t n);
void pone_ary_append(pone_universe* universe, pone_val* self, pone_val* val);
void pone_ary_append_noinc(pone_universe* universe, pone_val* self, pone_val* val);
void pone_ary_assign_pos(pone_world* world, pone_val* self, pone_val* pos, pone_val* val);

// str.c
pone_val* pone_str_new(pone_universe* universe, const char*p, size_t len);
pone_val* pone_str_new_const(pone_universe* universe, const char*p, size_t len);
void pone_str_free(pone_universe* universe, pone_val* val);
pone_val* pone_stringify(pone_world* world, pone_val* val);
pone_val* pone_str_from_num(pone_universe* universe, double n);
const char* pone_str_ptr(pone_val* val);
size_t pone_str_len(pone_val* val);
char* pone_strdup(pone_universe* universe, const char* src, size_t size);
void pone_str_append_c(pone_world* world, pone_val* str, const char* s, pone_int_t s_len);
void pone_str_append(pone_world* world, pone_val* str, pone_val* s);
void pone_str_appendf(pone_world* world, pone_val* str, const char* fmt, ...);
void pone_str_init(pone_universe* universe);
pone_val* pone_str_new_vprintf(pone_universe* universe, const char* fmt, va_list args);
pone_val* pone_str_new_printf(pone_universe* universe, const char* fmt, ...);
bool pone_str_contains_null(pone_universe* universe, pone_val* val);
pone_val* pone_str_c_str(pone_world* world, pone_val* val);
pone_val* pone_str_copy(pone_universe* universe, pone_val* val);

// code.c
pone_val* pone_code_new_c(pone_universe* universe, pone_funcptr_t func);
pone_val* pone_code_new(pone_world* world, pone_funcptr_t func);
pone_val* pone_code_call(pone_world* world, pone_val* code, pone_val* self, int n, ...);
pone_val* pone_code_vcall(pone_world* world, pone_val* code, pone_val* self, int n, va_list args);
void pone_code_free(pone_universe* universe, pone_val* v);
void pone_code_init(pone_universe* universe);

// int.c
pone_val* pone_str_from_int(pone_universe* universe, pone_int_t i);
pone_int_t pone_int_val(pone_val* val);
pone_val* pone_int_incr(pone_world* world, pone_val* i);
pone_val* pone_int_new(pone_universe* universe, pone_int_t i);
void pone_int_init(pone_universe* universe);

// SV ops
double pone_num_val(pone_val* val);
bool pone_bool_val(pone_val* val);
void pone_refcnt_dec(pone_universe* universe, pone_val* val);
void pone_refcnt_inc(pone_universe* universe, pone_val* val);
size_t pone_elems(pone_world* world, pone_val* val);
pone_int_t pone_intify(pone_world* world, pone_val* val);
pone_num_t pone_numify(pone_world* world, pone_val* val);
bool pone_is_frozen(pone_val* v);

// scope.c
pone_val* pone_mortalize(pone_world* world, pone_val* val);
void pone_push_scope(pone_world* world);
void pone_pop_scope(pone_world* world);
void pone_freetmps(pone_world* world);
void pone_savetmps(pone_world* world);
pone_lex_t* pone_lex_new(pone_world* world, pone_lex_t* parent);
void pone_lex_refcnt_dec(pone_world* world, pone_lex_t* lex);
void pone_lex_refcnt_inc(pone_world* world, pone_lex_t* lex);

// universe.c
pone_universe* pone_universe_init();
void pone_universe_destroy(pone_universe* universe);
void pone_universe_default_err_handler(pone_world* world);
void pone_universe_set_global(pone_universe* universe, const char* key, pone_val* val);

// bool.c
pone_val* pone_true();
pone_val* pone_false();
void pone_bool_init(pone_universe* universe);

// num.c
pone_val* pone_num_new(pone_universe* universe, double i);
void pone_num_init(pone_universe* universe);

// basic value operations
static inline int pone_refcnt(pone_val* val) { return val->as.basic.refcnt; }
static inline pone_t pone_type(pone_val* val) { return val->as.basic.type; }
static inline pone_t pone_flags(pone_val* val) { return val->as.basic.flags; }
static inline bool pone_defined(pone_val* val) { return val->as.basic.type != PONE_NIL; }

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
void pone_signal_handle(pone_world* world);
pone_val* pone_builtin_printf(pone_world* world, pone_val* fmt, ...);

void pone_val_free(pone_universe* universe, pone_val* p);
pone_t pone_type(pone_val* val);
void* pone_malloc(pone_universe* universe, size_t size);
pone_val* pone_obj_alloc(pone_universe* universe, pone_t type);
void pone_free(pone_universe* universe, void* p);

// cool.c
pone_val* pone_init_cool(pone_universe* universe);

// any.c
pone_val* pone_init_any(pone_universe* universe);

// class.c
pone_val* pone_init_class(pone_universe* universe);
pone_val* pone_class_new(pone_universe* universe, const char* name, size_t name_len);
void pone_class_push_parent(pone_universe* universe, pone_val* obj, pone_val* klass);
void pone_add_method(pone_universe* universe, pone_val* klass, const char* name, size_t name_len, pone_val* method);
void pone_add_method_c(pone_universe* universe, pone_val* klass, const char* name, size_t name_len, pone_funcptr_t funcptr);
pone_val* pone_find_method(pone_world* world, pone_val* klass, const char* name);
pone_val* pone_what(pone_universe* universe, pone_val* obj);
pone_val* pone_call_method(pone_world* world, pone_val* obj, const char* method_name, int n, ...);
pone_val* pone_call_meta_method(pone_world* world, pone_val* obj, const char* method_name, int n, ...);
void pone_class_compose(pone_universe* universe, pone_val* klass);

// obj.c
pone_val* pone_init_mu(pone_universe* universe);
pone_val* pone_obj_new(pone_universe* universe, pone_val* klass);
void pone_obj_free(pone_universe* universe, pone_val* val);
void pone_obj_set_ivar(pone_universe* universe, pone_val* obj, const char* name, pone_val* val);
void pone_obj_set_ivar_noinc(pone_universe* universe, pone_val* obj, const char* name, pone_val* val);
pone_val* pone_obj_get_ivar(pone_universe* universe, pone_val* obj, const char* name);

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
void pone_range_init(pone_universe* universe);

// socket.c
void pone_sock_init(pone_universe* universe);

// re.c
pone_val* pone_regex_new(pone_universe* universe, const char* str, size_t len);
void pone_regex_init(pone_universe* universe);
pone_val* pone_match_new(pone_universe* universe, pone_val* orig, pone_int_t from, pone_int_t to);
void pone_match_push(pone_world* world, pone_val* self, pone_int_t from, pone_int_t to);

// thread.c
void pone_thread_init(pone_universe* universe);
pone_val* pone_thread_join(pone_universe* universe, pthread_t thr);

// pair.c
void pone_pair_init(pone_universe* universe);
pone_val* pone_pair_new(pone_universe* universe, pone_val* key, pone_val* value);

#ifdef THREAD_DEBUG
#define THREAD_TRACE(fmt, ...) printf("[pone-thread] " fmt, ##__VA_ARGS__)
#else
#define THREAD_TRACE(fmt, ...)
#endif

#define GVL_LOCK(universe) \
  do { \
      THREAD_TRACE("LOCK: %d\n", pthread_self()); \
      pthread_mutex_lock(&(universe->mutex)); \
  } while (0)
#define GVL_UNLOCK(universe) \
  do { \
      THREAD_TRACE("UNLOCK: %d\n", pthread_self()); \
      pthread_mutex_unlock(&(universe->mutex)); \
  } while(0)
#define PONE_YIELD(universe) \
    do { \
        GVL_UNLOCK(universe); \
        if ((sched_yield()) == -1) { \
            perror("cannot yield thread"); \
            exit(EXIT_FAILURE); \
        } \
        GVL_LOCK(universe); \
    } while (0)

#define PONE_ALLOC_CHECK(v) \
  do { \
    if (!v) { \
      fprintf(stderr, "Cannot allocate memory\n"); \
      abort(); \
    } \
  } while (0)

#define PONE_DECLARE_GETTER(name, var) \
    static pone_val* name(pone_world* world, pone_val* self, int n, va_list args) { \
        assert(n == 0); \
        pone_val* v = pone_obj_get_ivar(world->universe, self, var); \
        pone_refcnt_inc(world->universe, v); \
        return v; \
    }


#endif

