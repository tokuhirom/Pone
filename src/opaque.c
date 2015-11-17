#include "pone.h"

pone_val* pone_opaque_new(pone_world* world, void* ptr, pone_finalizer_t finalizer) {
    pone_val* opaque = pone_obj_alloc(world, PONE_OPAQUE);
    opaque->as.opaque.ptr = ptr;
    opaque->as.opaque.finalizer = finalizer;
    return opaque;
}

void pone_opaque_free(pone_world* world, pone_val* v) {
    assert(pone_type(v) == PONE_OPAQUE);
    
    pone_finalizer_t finalizer = v->as.opaque.finalizer;
    if (finalizer) {
        finalizer(world, v);
    }
}

void pone_opaque_init(pone_world* world) {
    pone_universe* universe = world->universe;

    pone_val* klass = pone_class_new(world, "Opaque", strlen("Opaque"));
    pone_class_push_parent(world, klass, universe->class_any);
    pone_class_compose(world, klass);

    universe->class_opaque = klass;
}

