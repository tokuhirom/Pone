#include "pone.h" /* PONE_INC */

void pone_obj_mark(pone_val* val) {
    assert(pone_type(val) == PONE_OBJ);
    assert(val->as.obj.ivar);
    assert(val->as.obj.klass);

    const char* k;
    pone_val* v;
    kh_foreach(val->as.obj.ivar, k, v, {
        pone_gc_mark_value(v);
    });
    pone_gc_mark_value(val->as.obj.klass);
}

pone_val* pone_obj_new(pone_world* world, pone_val* klass) {
    assert(klass);
    assert(pone_type(klass) == PONE_OBJ);

    pone_obj* obj = (pone_obj*)pone_obj_alloc(world, PONE_OBJ);
    obj->klass = klass;
    obj->ivar = kh_init(str);

    return (pone_val*)obj;
}

inline void pone_obj_free(pone_world* world, pone_val* val) {
    assert(pone_type(val) == PONE_OBJ);

    // free instance variables
    const char* k;
    pone_val* v;
    kh_foreach(val->as.obj.ivar, k, v, {
        pone_free(world->universe, (void*)k); // k is strdupped.
    });

    kh_destroy(str, val->as.obj.ivar);
}

// set instance variable to the object without refinc++
void pone_obj_set_ivar(pone_world* world, pone_val* obj, const char* name, pone_val* val) {
    assert(pone_type(obj) == PONE_OBJ);

    char* ks = pone_strdup(world, name, strlen(name));
    int ret;
    khint_t key = kh_put(str, obj->as.obj.ivar, ks, &ret);
    if (ret == -1) {
        fprintf(stderr, "[ERROR] cannot put value to instance variable\n");
        abort();
    }
    kh_val(obj->as.obj.ivar, key) = val;
}

// get instance variable from the object
pone_val* pone_obj_get_ivar(pone_world* world, pone_val* obj, const char* name) {
    assert(pone_type(obj) == PONE_OBJ);

    khint_t entry = kh_get(str, obj->as.obj.ivar, name);
    if (kh_end(obj->as.obj.ivar) != entry) {
        return kh_val(obj->as.obj.ivar, entry);
    } else {
        return pone_nil();
    }
}

pone_val* pone_obj_copy(pone_world* world, pone_val* obj) {
    pone_val* retval = pone_obj_new(world, obj->as.obj.klass);
    const char* k;
    pone_val* v;
    kh_foreach(obj->as.obj.ivar, k, v, {
        pone_obj_set_ivar(world, retval, k, v);
    });
    return retval;
}

// deep copy object to current world.
pone_val* pone_val_copy(pone_world* world, pone_val* obj) {
#ifndef NDEBUG
    if (!pone_alive(obj)) {
        fprintf(stderr, "[BUG] can't copy freed' value: ptr:%p thread:%lx\n", obj, pthread_self());
        abort();
    }
#endif

    switch (pone_type(obj)) {
    case PONE_NIL: // nil is singleton.global object
        return obj;
    case PONE_BOOL: // bool is singleton.global object
        return obj;
    case PONE_INT:
        return pone_int_new(world, pone_int_val(obj));
    case PONE_NUM:
        return pone_num_new(world, pone_num_val(obj));
    case PONE_STRING:
        if (obj->as.basic.flags & PONE_FLAGS_STR_UTF8) {
            return pone_str_new_strdup(world, pone_str_ptr(obj), pone_str_len(obj));
        } else {
            return pone_bytes_new_strdup(world, pone_str_ptr(obj), pone_str_len(obj));
        }
    case PONE_ARRAY:
        return pone_ary_copy(world, obj);
    case PONE_HASH:
        return pone_hash_copy(world, obj);
    case PONE_CODE:
        // TODO support code object copy...........
        // But, we can't copy lex object
        pone_throw_str(world, "You can't copy code object.");
        abort();
    case PONE_OPAQUE:
        return pone_opaque_new(world, pone_opaque_ptr(obj), NULL);
    case PONE_OBJ:
        return pone_obj_copy(world, obj);
    case PONE_LEX:
        abort();
    }
    abort();
}
