#ifndef PONE_OPAQUE_H_
#define PONE_OPAQUE_H_

#include "pone.h"

struct pone_opaque_body {
    struct pone_val* klass;
    void* ptr;
    pone_finalizer_t finalizer;
    pone_int_t refcnt;
};

void pone_opaque_init(pone_world* world);
pone_val* pone_opaque_copy(pone_world* world, pone_val* v);
pone_val* pone_opaque_new(pone_world* world, void* ptr, pone_finalizer_t finalizer);
static inline void pone_opaque_set_class(pone_world* world, pone_val* v, pone_val* klass) {
    v->as.opaque.body->klass = klass;
}
void pone_opaque_free(pone_world* world, pone_val* v);
static inline void* pone_opaque_ptr(pone_val* v) {
    return v->as.opaque.body->ptr;
}

#endif
