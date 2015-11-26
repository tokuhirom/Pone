#ifndef PONE_VAL_VEC_H_
#define PONE_VAL_VEC_H_

#include "pone.h"

static inline void pone_val_vec_init(struct pone_val_vec* vec) {
    vec->a = NULL;
    vec->n = 0;
    vec->m = 0;
}

static inline void pone_val_vec_push(struct pone_val_vec* vec, struct pone_val* v) {
    if (vec->n == vec->m) {
        vec->m = vec->m == 0 ? 2 : vec->m << 1;
        vec->a = realloc(vec->a, sizeof(struct pone_val*) * vec->m);
    }
    vec->a[vec->n++] = v;
}

static inline void pone_val_vec_destroy(struct pone_val_vec* vec) {
    free(vec->a);
}

static inline struct pone_val* pone_val_vec_last(struct pone_val_vec* vec) {
    return vec->a[vec->n - 1];
}

static inline void pone_val_vec_pop(struct pone_val_vec* vec) {
    assert(vec->n > 0);
    vec->n--;
}

static inline struct pone_val* pone_val_vec_get(struct pone_val_vec* vec, pone_int_t i) {
    assert(vec->n > i);
    return vec->a[i];
}

static inline pone_int_t pone_val_vec_size(struct pone_val_vec* vec) {
    return vec->n;
}

// delete item at i.
static inline void pone_val_vec_delete(struct pone_val_vec* vec, pone_int_t i) {
    assert(vec->n > i);
    // i=0
    // before: x a a a
    // after:  a a a
    //
    // i=1
    // before: a x a a
    // after:  a a a
    memmove(vec->a + i, vec->a + i + 1, vec->n - i - 1);
    vec->n--;
}

#endif
