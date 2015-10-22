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

// string literal is constant
#define PONE_FLAGS_STR_CONST 1
// This object is immutable
#define PONE_FLAGS_STR_FROZEN 2

typedef enum {
    PONE_UNDEF,
    PONE_INT,
    PONE_NUM,
    PONE_STRING,
    PONE_ARRAY,
    PONE_BOOL,
    PONE_HASH
} pone_t;

#define PONE_HEAD \
    int refcnt; \
    pone_t type; \
    uint8_t flags

typedef struct {
    PONE_HEAD;
} pone_val;

KHASH_MAP_INIT_STR(str, pone_val*)

typedef struct {
    PONE_HEAD;
} pone_undef;

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
    pone_val** a;
    size_t max;
    size_t len;
} pone_ary;

typedef struct {
    PONE_HEAD;
    khash_t(str) *h;
    size_t len;
} pone_hash;

typedef struct lex_entry {
    struct lex_entry* parent;
    khash_t(str) *map;
} lex_entry;

typedef struct {
    // save last tmpstack_floor
    size_t* savestack;
    size_t savestack_idx;
    size_t savestack_max;

    // mortals we've made
    pone_val** tmpstack;
    size_t tmpstack_idx;
    size_t tmpstack_floor;
    size_t tmpstack_max;

    // lexical value stack
    lex_entry* lex;
} pone_world;

static pone_val pone_undef_val = { -1, PONE_UNDEF, 0 };

// hash.c
pone_val* pone_new_hash(pone_world* world, int n, ...);
void pone_hash_put(pone_world* world, pone_val* hv, pone_val* k, pone_val* v);
size_t pone_hash_elems(pone_val* val);
pone_val* pone_hash_free(pone_world* world, pone_val* val);

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

// SV ops
int pone_int_val(pone_val* val);
double pone_num_val(pone_val* val);
bool pone_bool_val(pone_val* val);
void pone_refcnt_dec(pone_world* world, pone_val* val);
void pone_refcnt_inc(pone_world* world, pone_val* val);

// scope
pone_val* pone_mortalize(pone_world* world, pone_val* val);

pone_val* pone_true();
pone_val* pone_false();

pone_val* pone_new_int(pone_world* world, int i);
pone_val* pone_new_num(pone_world* world, double i);


pone_t pone_type(pone_val* val);
void* pone_malloc(pone_world* world, size_t size);
void pone_free(pone_world* world, void* p);
void pone_die(pone_world* world, const char* str);

#endif

