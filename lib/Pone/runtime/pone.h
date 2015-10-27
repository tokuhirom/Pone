#ifndef PONE_H_

#define PONE_H_

#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdarg.h>
#include "khash.h" /* PONE_INC */

// TODO: NaN boxing

// This variable is global var.
#define PONE_FLAGS_GLOBAL 1
// string literal is constant
#define PONE_FLAGS_STR_CONST 2
// This object is immutable
#define PONE_FLAGS_STR_FROZEN 4

typedef enum {
    PONE_NIL=1,
    PONE_INT,
    PONE_NUM,
    PONE_STRING,
    PONE_ARRAY,
    PONE_BOOL,
    PONE_HASH,
    PONE_CODE
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
    int i;
} pone_int;

typedef struct {
    PONE_HEAD;
    double n; 
} pone_num;

typedef struct {
    PONE_HEAD;
    const char* p;
    size_t len;
} pone_string;

typedef struct {
    PONE_HEAD;
    bool b;
} pone_bool;

typedef struct {
    PONE_HEAD;
    struct pone_val** a;
    size_t max;
    size_t len;
} pone_ary;

typedef struct {
    PONE_HEAD;
    khash_t(str) *h;
    size_t len;
} pone_hash;

typedef struct pone_lex_t {
    PONE_HEAD;
    struct pone_lex_t* parent;
    khash_t(str) *map;
} pone_lex_t;

#ifndef PONE_ARENA_SIZE
#define PONE_ARENA_SIZE 1024
#endif

struct pone_arena;

// VM context
typedef struct pone_universe {
    struct pone_arena* arena_head;
    struct pone_arena* arena_last;

    // list of unused values.
    struct pone_val* freelist;
} pone_universe;

// Calling context
typedef struct pone_world {
    pone_universe* universe;

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
} pone_world;

typedef struct pone_val* (*pone_funcptr_t)(pone_world*, int n, va_list);

typedef struct {
    PONE_HEAD;
    pone_funcptr_t func;
    pone_lex_t* lex;
} pone_code;

typedef struct pone_val {
    union {
        // see http://loveruby.net/ja/rhg/book/gc.html
        struct {
            struct pone_val* next; // next free value
        } free;

        pone_basic basic;
        pone_ary ary;
        pone_code code;
        pone_hash hash;
    } as;
} pone_val;

typedef struct pone_arena {
    struct pone_arena* next;
    int idx;
    struct pone_val values[PONE_ARENA_SIZE];
} pone_arena;


// nil.c
pone_val* pone_nil();

// world.c
pone_world* pone_new_world(pone_universe* universe);
pone_world* pone_new_world_from_world(pone_world* world, pone_lex_t* lex);

// op.c
void pone_dd(pone_world* world, pone_val* val);
const char* pone_what_str_c(pone_val* val);

// hash.c
pone_val* pone_new_hash(pone_world* world, int n, ...);
void pone_hash_put(pone_world* world, pone_val* hv, pone_val* k, pone_val* v);
size_t pone_hash_elems(pone_val* val);
void pone_hash_free(pone_world* world, pone_val* val);

// array.c
pone_val* pone_new_ary(pone_world* world, int n, ...);
size_t pone_ary_elems(pone_val* val);
pone_val* pone_ary_at_pos(pone_val* val, int pos);
void pone_ary_free(pone_world* world, pone_val* val);

// str.c
pone_val* pone_new_str(pone_world* world, const char*p, size_t len);
pone_val* pone_new_str_const(pone_world* world, const char*p, size_t len);
void pone_str_free(pone_world* world, pone_val* val);
pone_val* pone_str(pone_world* world, pone_val* val);
pone_val* pone_str_from_num(pone_world* world, double n);
const char* pone_string_ptr(pone_val* val);
size_t pone_string_len(pone_val* val);

// code.c
pone_val* pone_code_new(pone_world* world, pone_funcptr_t func);
pone_val* pone_code_call(pone_world* world, pone_val* code, int n, ...);
void pone_code_free(pone_world* world, pone_val* v);

// SV ops
int pone_int_val(pone_val* val);
double pone_num_val(pone_val* val);
bool pone_bool_val(pone_val* val);
void pone_refcnt_dec(pone_world* world, pone_val* val);
void pone_refcnt_inc(pone_world* world, pone_val* val);

// scope.c
pone_val* pone_mortalize(pone_world* world, pone_val* val);
void pone_push_scope(pone_world* world);
void pone_pop_scope(pone_world* world);
void pone_freetmps(pone_world* world);
void pone_savetmps(pone_world* world);
pone_lex_t* pone_lex_new(pone_world* world, pone_lex_t* parent);
void pone_lex_refcnt_dec(pone_world* world, pone_lex_t* lex);
void pone_lex_refcnt_inc(pone_world* world, pone_lex_t* lex);

// alloc.c
pone_universe* pone_universe_init();
void pone_universe_destroy(pone_universe* universe);

// bool.c
pone_val* pone_true();
pone_val* pone_false();

pone_val* pone_new_int(pone_world* world, int i);
pone_val* pone_new_num(pone_world* world, double i);

// basic value operations
static inline int pone_refcnt(pone_val* val) { return val->as.basic.refcnt; }
static inline pone_t pone_type(pone_val* val) { return val->as.basic.type; }
static inline pone_t pone_flags(pone_val* val) { return val->as.basic.flags; }

void pone_obj_free(pone_world* world, pone_val* p);
pone_t pone_type(pone_val* val);
void* pone_malloc(pone_world* world, size_t size);
pone_val* pone_obj_alloc(pone_world* world, pone_t type);
void pone_free(pone_world* world, void* p);
void pone_die(pone_world* world, const char* str);

#endif

