#include "pone.h" /* PONE_INC */

/**
 * Class related operations
 */

// initialize Class class
pone_val* pone_init_class(pone_world* world) {
    pone_val* val = pone_obj_alloc(world, PONE_OBJ);
    val->as.obj.ivar = kh_init(str);
    val->as.obj.klass = pone_nil();

    pone_obj_set_ivar(world, val, "$!name", pone_str_new_const(world, "Class", strlen("class")));
    pone_obj_set_ivar(world, val, "@!parents", pone_ary_new(world, 0));
    return val;
}

pone_val* pone_what(pone_world* world, pone_val* obj) {
    assert(pone_alive(obj));
    pone_universe* universe = world->universe;

    switch (pone_type(obj)) {
    case PONE_NIL:
        return universe->class_nil;
    case PONE_INT:
        return universe->class_int;
    case PONE_NUM:
        return universe->class_num;
    case PONE_STRING:
        return universe->class_str;
    case PONE_ARRAY:
        return universe->class_ary;
    case PONE_BOOL:
        return universe->class_bool;
    case PONE_HASH:
        return universe->class_hash;
    case PONE_CODE:
        return universe->class_code;
    case PONE_OPAQUE:
        if (obj->as.opaque.klass != NULL) {
            return obj->as.opaque.klass;
        } else {
            return universe->class_opaque;
        }
    case PONE_OBJ:
        if (obj->as.obj.klass == universe->class_class) {
            return obj; // return obj itself if it's a class.
        } else {
            return obj->as.obj.klass;
        }
    case PONE_LEX:
        abort();
    }
    abort();
}

/**
 * Create new instance of class
 */
pone_val* pone_class_new(pone_world* world, const char* name, size_t name_len) {
    pone_val* obj = pone_obj_new(world, world->universe->class_class);
    pone_obj_set_ivar(world, (pone_val*)obj, "$!name", pone_str_new(world, name, name_len));
    pone_obj_set_ivar(world, (pone_val*)obj, "@!parents", pone_ary_new(world, 0));

    return (pone_val*)obj;
}

void pone_class_push_parent(pone_world* world, pone_val* obj, pone_val* klass) {
    pone_val* parents = pone_obj_get_ivar(world, obj, "@!parents");
    pone_ary_push(world->universe, parents, klass);
}

static void _compose(pone_world* world, khash_t(str) *target_methods, pone_val* klass) {
    const char* k;
    pone_val* v;
    kh_foreach(klass->as.obj.ivar, k, v, {
        assert(v);
        khint_t h = kh_get(str, target_methods, k);
        int ret;
        if (kh_end(target_methods) == h) {
            khint_t h2 = kh_put(str, target_methods, k, &ret);
            kh_val(target_methods, h2) = v;
        }
    });

    pone_val* parents = pone_obj_get_ivar(world, klass, "@!parents");
    assert(pone_type(parents) == PONE_ARRAY);
    pone_int_t l = pone_ary_elems(parents);
    for (pone_int_t i=0; i<l; ++i) {
        _compose(world, target_methods, pone_ary_at_pos(parents, i));
    }
}

// .^compose
void pone_class_compose(pone_world* world, pone_val* klass) {
    _compose(world, klass->as.obj.ivar, klass);
}

// rename to pone_get_attr?
pone_val* pone_find_method(pone_world* world, pone_val* obj, const char* name) {
    assert(pone_alive(obj));

    pone_val* klass = pone_what(world, obj);
    assert(klass);
    return pone_obj_get_ivar(world, klass, name);
}

// Usage: return pone_call_method(world, iter, "pull-one", 0);
pone_val* pone_call_method(pone_world* world, pone_val* obj, const char* method_name, int n, ...) {
    assert(obj);
#ifndef NDEBUG
    if (!pone_alive(obj)) {
        fprintf(stderr, "value %p is already free'd\n", obj);
        abort();
    }
#endif

    pone_val* method = pone_find_method(world, obj, method_name);
    if (pone_defined(method)) {
        va_list args;

        va_start(args, n);

        pone_val* retval = pone_code_vcall(world, method, obj, n, args);

        va_end(args);
        return retval;
    } else {
        pone_throw_str(world, "Method '%s' not found for invocant of class '%s'", method_name, pone_what_str_c(obj));
        abort();
    }
}

pone_val* pone_call_meta_method(pone_world* world, pone_val* obj, const char* method_name, int n, ...) {
    if (strcmp(method_name, "methods") == 0) {
        // .^methods
        pone_val* klass = pone_what(world, obj);
        pone_val* methods = pone_ary_new(world, 0);
        const char* key;
        pone_val* val;
        kh_foreach(klass->as.obj.ivar, key, val, {
            pone_ary_push(world->universe, methods, val);
        });
        return methods;
    } else {
        pone_throw_str(world, "Meta method '%s' not found for invocant of class '%s'", method_name, pone_what_str_c(obj));
        abort();
    }
}

