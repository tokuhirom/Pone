#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdbool.h>
#include <stdarg.h>
#include "khash.h" /* PONE_INC */

// TODO: NaN boxing

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
    pone_t type;

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
} pone_world;

static pone_val pone_undef_val = { -1, PONE_UNDEF };
static pone_bool pone_true_val = { -1, PONE_BOOL, true };
static pone_bool pone_false_val = { -1, PONE_BOOL, false };

// SV ops
int pone_int_val(pone_val* val);
double pone_num_val(pone_val* val);
bool pone_bool_val(pone_val* val);
const char* pone_string_ptr(pone_val* val);
size_t pone_string_len(pone_val* val);
void pone_refcnt_dec(pone_world* world, pone_val* val);
void pone_refcnt_inc(pone_world* world, pone_val* val);

pone_val* pone_new_ary(pone_world* world, int n, ...);
pone_val* pone_new_hash(pone_world* world, int n, ...);
void pone_hash_put(pone_world* world, pone_val* hv, pone_val* k, pone_val* v);
size_t pone_hash_elems(pone_val* val);
size_t pone_ary_elems(pone_val* val);
pone_val* pone_ary_at_pos(pone_val* val, int pos);

// scope
pone_val* pone_mortalize(pone_world* world, pone_val* val);

pone_val* pone_true();
pone_val* pone_false();

pone_val* pone_str_from_num(pone_world* world, double n);
pone_val* pone_new_int(pone_world* world, int i);
pone_val* pone_new_num(pone_world* world, double i);
pone_val* pone_new_str(pone_world* world, const char*p, size_t len);
pone_val* pone_str(pone_world* world, pone_val* val);
pone_t pone_type(pone_val* val);
void* pone_malloc(pone_world* world, size_t size);
void pone_free(pone_world* world, void* p);
void pone_die(pone_world* world, const char* str);

inline pone_val* pone_true() {
    return (pone_val*)&pone_true_val;
}

inline pone_val* pone_false() {
    return (pone_val*)&pone_false_val;
}

pone_world* pone_new_world() {
    // we can't use pone_malloc yet.
    pone_world* world = malloc(sizeof(pone_world));
    if (!world) {
        fprintf(stderr, "Cannot make world\n");
    }
    memset(world, 0, sizeof(pone_world));

    world->savestack = malloc(sizeof(size_t*) * 64);
    world->savestack_max = 64;

    world->tmpstack = malloc(sizeof(size_t*) * 64);
    world->tmpstack_max = 64;

    return world;
}

void pone_destroy_world(pone_world* world) {
    free(world->savestack);
    free(world->tmpstack);
    free(world);
}

void pone_dd(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
        case PONE_STRING:
            printf("(string: ");
            fwrite(pone_string_ptr(val), 1, pone_string_len(val), stdout);
            printf(")\n");
            break;
        case PONE_INT:
            printf("(int: %d)\n", pone_int_val(val));
            break;
        case PONE_UNDEF:
            printf("(undef)\n");
            break;
        default:
            abort();
    }
}

pone_val*  pone_builtin_dd(pone_world* world, pone_val* val) {
    pone_dd(world, val);
    return &pone_undef_val;
}

pone_val*  pone_builtin_abs(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_INT: {
        int i = pone_int_val(val);
        if (i < 0) {
            return pone_mortalize(world, pone_new_int(world, -i));
        } else {
            return val;
        }
    }
                   // TODO: NV
    }

    pone_die(world, "you can't call abs() for non-numeric value");
}

inline pone_t pone_type(pone_val* val) {
    return val->type;
}

inline void pone_refcnt_inc(pone_world* world, pone_val* val) {
    assert(val != NULL);

    val->refcnt++;
}

// decrement reference count
inline void pone_refcnt_dec(pone_world* world, pone_val* val) {
    assert(val != NULL);

    val->refcnt--;
    if (val->refcnt == 0) {
        switch (pone_type(val)) {
        case PONE_STRING:
            pone_free(world, (char*)((pone_string*)val)->p);
            break;
        case PONE_ARRAY: {
            pone_ary* a=(pone_ary*)val;
            size_t l = pone_ary_elems(val);
            for (int i=0; i<l; ++i) {
                pone_refcnt_dec(world, a->a[i]);
            }
            pone_free(world, a->a);
            break;
        }
        case PONE_HASH: {
            pone_hash* h=(pone_hash*)val;
            const char* k;
            pone_val* v;
            kh_foreach(h->h, k, v, {
                pone_free(world, (void*)k); // k is strdupped.
                pone_refcnt_dec(world, v);
            });
            kh_destroy(str, h->h);
            break;
        }
        }
        pone_free(world, val);
    }
}

bool pone_so(pone_val* val) {
    switch (pone_type(val)) {
    case PONE_INT:
        return pone_int_val(val) != 0;
    case PONE_NUM:
        return pone_int_val(val) != 0;
    case PONE_STRING:
        return pone_string_len(val) != 0;
    case PONE_BOOL:
        return pone_bool_val(val);
    default:
        return true;
    }
}

inline const char* pone_string_ptr(pone_val* val) {
    assert(pone_type(val) == PONE_STRING);
    return ((pone_string*)val)->p;
}

inline size_t pone_string_len(pone_val* val) {
    assert(pone_type(val) == PONE_STRING);
    return ((pone_string*)val)->len;
}

inline int pone_int_val(pone_val* val) {
    assert(pone_type(val) == PONE_INT);
    return ((pone_int*)val)->i;
}

inline double pone_num_val(pone_val* val) {
    assert(pone_type(val) == PONE_NUM);
    return ((pone_num*)val)->n;
}

inline bool pone_bool_val(pone_val* val) {
    assert(pone_type(val) == PONE_BOOL);
    return ((pone_bool*)val)->b;
}

void pone_die(pone_world* world, const char* str) {
    fprintf(stderr, "%s\n", str);
    exit(1);
}

int pone_to_int(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_UNDEF:
        pone_die(world, "Use of uninitialized value as integer");
        break;
    case PONE_INT:
        return pone_int_val(val);
    case PONE_STRING: {
        char *end = (char*)pone_string_ptr(val) + pone_string_len(val);
        return strtol(pone_string_ptr(val), &end, 10);
    }
    default:
        abort();
    }
}

// TODO: support NV
pone_val* pone_add(pone_world* world, pone_val* v1, pone_val* v2) {
    int i1 = pone_to_int(world, v1);
    int i2 = pone_to_int(world, v2);
    return pone_mortalize(world, pone_new_int(world, i1 + i2));
}

// TODO: support NV
pone_val* pone_subtract(pone_world* world, pone_val* v1, pone_val* v2) {
    int i1 = pone_to_int(world, v1);
    int i2 = pone_to_int(world, v2);
    return pone_mortalize(world, pone_new_int(world, i1 - i2));
}

pone_val* pone_multiply(pone_world* world, pone_val* v1, pone_val* v2) {
    int i1 = pone_to_int(world, v1);
    int i2 = pone_to_int(world, v2);
    return pone_mortalize(world, pone_new_int(world, i1 * i2));
}

pone_val* pone_divide(pone_world* world, pone_val* v1, pone_val* v2) {
    int i1 = pone_to_int(world, v1);
    int i2 = pone_to_int(world, v2);
    return pone_mortalize(world, pone_new_int(world, i1 / i2)); // TODO: We should upgrade value to NV
}

pone_val* pone_str_from_int(pone_world* world, int i) {
    // INT_MAX=2147483647. "2147483647".elems = 10
    char buf[11+1];
    int size = snprintf(buf, 11+1, "%d", i);
    return pone_mortalize(world, pone_new_str(world, buf, size));
}

pone_val* pone_str_from_num(pone_world* world, double n) {
    char buf[512+1];
    int size = snprintf(buf, 512+1, "%f", n);
    return pone_mortalize(world, pone_new_str(world, buf, size));
}

/**
 * @return (mortalized)
 */
pone_val* pone_str(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_UNDEF:
        return pone_mortalize(world, pone_new_str(world, "(undef)", strlen("(undef)")));
    case PONE_INT:
        return pone_str_from_int(world, pone_int_val(val));
    case PONE_NUM:
        return pone_str_from_num(world, pone_num_val(val));
    case PONE_STRING:
        return val;
    case PONE_BOOL:
        if (pone_bool_val(val)) {
            return pone_mortalize(world, pone_new_str(world, "True", strlen("True")));
        } else {
            return pone_mortalize(world, pone_new_str(world, "False", strlen("False")));
        }
    default:
        abort();
    }
}

pone_val* pone_builtin_print(pone_world* world, pone_val* val) {
    pone_val* str = pone_str(world, val);
    fwrite(pone_string_ptr(str), sizeof(char), pone_string_len(str), stdout);
    return &pone_undef_val;
}

pone_val* pone_builtin_say(pone_world* world, pone_val* val) {
    pone_builtin_print(world, val);
    fwrite("\n", sizeof(char), 1, stdout);
    return &pone_undef_val;
}

size_t pone_elems(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_STRING:
        return pone_string_len(val);
    case PONE_ARRAY:
        return pone_ary_elems(val);
    case PONE_HASH:
        return pone_hash_elems(val);
    }
    return 1;
}

pone_val* pone_builtin_elems(pone_world* world, pone_val* val) {
    return pone_mortalize(world, pone_new_int(world, pone_elems(world, val)));
}

// TODO: implement memory pool
void* pone_malloc(pone_world* world, size_t size) {
    void* p = malloc(size);
    if (!p) {
        fprintf(stderr, "Cannot allocate memory\n");
        exit(1);
    }
    memset(p, 0, size);
    return p;
}

void pone_free(pone_world* world, void* p) {
    free(p);
}

const char* pone_strdup(pone_world* world, const char* src, size_t size) {
    char* p = (char*)malloc(size+1);
    if (!p) {
        fprintf(stderr, "Cannot allocate memory\n");
        exit(1);
    }
    memcpy(p, src, size);
    p[size] = '\0';
    return p;
}

pone_val* pone_mortalize(pone_world* world, pone_val* val) {
    world->tmpstack[world->tmpstack_idx] = val;
    ++world->tmpstack_idx;
    if (world->tmpstack_idx > world->tmpstack_max) {
        world->tmpstack_max *= 2;
        pone_val** ssp = realloc(world->tmpstack, sizeof(pone_val*)*world->tmpstack_max);
        if (!ssp) {
            pone_die(world, "Cannot allocate memory");
        }
        world->tmpstack = ssp;
    }
    return val;
}

pone_val* pone_new_ary(pone_world* world, int n, ...) {
    va_list list;

    pone_ary* av = (pone_ary*)pone_malloc(world, sizeof(pone_ary));
    av->refcnt = 1;
    av->type   = PONE_ARRAY;

    va_start(list, n);
    av->a = (pone_val**)pone_malloc(world, sizeof(pone_ary)*n);
    av->max = n;
    av->len = n;
    // we can optimize in case of `[1,2,3]`.
    for (int i=0; i<n; ++i) {
        pone_val* v = va_arg(list, pone_val*);
        av->a[i] = v;
        pone_refcnt_inc(world, v);
    }
    va_end(list);

    return (pone_val*)av;
}

// TODO: delete key
// TODO: push key
// TODO: exists key
pone_val* pone_new_hash(pone_world* world, int n, ...) {
    va_list list;

    pone_hash* hv = (pone_hash*)pone_malloc(world, sizeof(pone_hash));
    hv->refcnt = 1;
    hv->type   = PONE_HASH;

    hv->h = kh_init(str);

    va_start(list, n);
    // we can optimize in case of `{a => 3}`. we can omit mortalize.
    for (int i=0; i<n; i+=2) {
        pone_val* k = va_arg(list, pone_val*);
        pone_val* v = va_arg(list, pone_val*);
        pone_hash_put(world, (pone_val*)hv, k, v);
    }
    va_end(list);

    return (pone_val*)hv;
}

void pone_hash_put(pone_world* world, pone_val* hv, pone_val* k, pone_val* v) {
    assert(pone_type(hv) == PONE_HASH);
    k = pone_str(world, k);
    // TODO: check ret
    int ret;
    const char* ks=pone_strdup(world, pone_string_ptr(k), pone_string_len(k));
    khint_t key = kh_put(str, ((pone_hash*)hv)->h, ks, &ret);
    kh_val(((pone_hash*)hv)->h, key) = v;
    pone_refcnt_inc(world, v);
    ((pone_hash*)hv)->len++;
}

size_t pone_hash_elems(pone_val* val) {
    assert(pone_type(val) == PONE_HASH);
    return ((pone_hash*)val)->len;
}

pone_val* pone_ary_at_pos(pone_val* av, int i) {
    assert(pone_type(av) == PONE_ARRAY);
    pone_ary*a = (pone_ary*)av;
    if (a->len > i) {
        return a->a[i];
    } else {
        return &pone_undef_val;
    }
}

size_t pone_ary_elems(pone_val* av) {
    assert(pone_type(av) == PONE_ARRAY);
    return ((pone_ary*)av)->len;
}

pone_val* pone_new_int(pone_world* world, int i) {
    pone_int* iv = (pone_int*)pone_malloc(world, sizeof(pone_int));
    iv->refcnt = 1;
    iv->type   = PONE_INT;
    iv->i = i;
    return (pone_val*)iv;
}

pone_val* pone_new_num(pone_world* world, double n) {
    pone_num* nv = (pone_num*)pone_malloc(world, sizeof(pone_num));
    nv->refcnt = 1;
    nv->type   = PONE_NUM;
    nv->n = n;
    return (pone_val*)nv;
}

pone_val* pone_new_str(pone_world* world, const char*p, size_t len) {
    pone_string* pv = (pone_string*)pone_malloc(world, sizeof(pone_string));
    pv->refcnt = 1;
    pv->type = PONE_STRING;
    pv->p = pone_strdup(world, p, len);
    pv->len = len;
    return (pone_val*)pv;
}

void pone_enter(pone_world* world) {
    // save original tmpstack_floor
    world->savestack[world->savestack_idx] = world->tmpstack_floor;
    ++world->savestack_idx;
    if (world->savestack_max+1 < world->savestack_idx) {
        // grow it
        world->savestack_max *= 2;
        size_t* ssp = realloc(world->savestack, sizeof(size_t)*world->savestack_max);
        if (!ssp) {
            pone_die(world, "Cannot allocate memory");
        }
        world->savestack = ssp;
    }

    // save current tmpstack_idx
    world->tmpstack_floor = world->tmpstack_idx;
}

void pone_leave(pone_world* world) {
    // decrement refcnt for mortalized values
    while (world->tmpstack_idx > world->tmpstack_floor) {
        pone_refcnt_dec(world, world->tmpstack[world->tmpstack_idx-1]);
        --world->tmpstack_idx;
    }

    // restore tmpstack_floor
    world->tmpstack_floor = world->savestack[world->savestack_idx-1];

    // pop tmpstack_floor
    --world->savestack_idx;
}

