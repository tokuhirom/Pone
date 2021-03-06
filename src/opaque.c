#include "pone.h"
#include "pone_opaque.h"

// @args finalizer allows NULL.
pone_val* pone_opaque_new(pone_world* world, pone_val* klass, void* ptr, pone_finalizer_t finalizer) {
    struct pone_opaque_body* body = pone_malloc(world, sizeof(struct pone_opaque_body));
    body->ptr = ptr;
    body->finalizer = finalizer;
    body->refcnt = 1;
    body->klass = klass;

    pone_val* opaque = pone_obj_alloc(world, PONE_OPAQUE);
    opaque->as.opaque.body = body;
    return opaque;
}

pone_val* pone_opaque_copy(pone_world* world, pone_val* v) {
    struct pone_opaque_body* body = v->as.opaque.body;

    pone_val* opaque = pone_obj_alloc(world, PONE_OPAQUE);
    opaque->as.opaque.body = body;
    body->refcnt++;
    return opaque;
}

void pone_opaque_free(pone_world* world, pone_val* v) {
    assert(pone_type(v) == PONE_OPAQUE);

    v->as.opaque.body->refcnt--;
    if (v->as.opaque.body->refcnt == 0) {
        pone_finalizer_t finalizer = v->as.opaque.body->finalizer;
        // finalizer is optional. it may null value.
        if (finalizer) {
            finalizer(world, v);
        }
    }
    pone_free(world, v->as.opaque.body);
}
