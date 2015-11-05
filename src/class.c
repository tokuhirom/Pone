#include "pone.h" /* PONE_INC */

/**
 * Class related operations
 */

// initialize Class class
pone_val* pone_init_class(pone_universe* universe) {
    pone_val* val = pone_obj_alloc(universe, PONE_OBJ);
    val->as.obj.ivar = kh_init(str);
    return val;
}

pone_val* pone_what(pone_universe* universe, pone_val* obj) {
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
    case PONE_OBJ:
        return obj->as.obj.klass;
    }
}

/**
 * Create new instance of class
 */
pone_val* pone_class_new(pone_universe* universe, const char* name, size_t name_len) {
    pone_val* obj = pone_obj_new(universe, universe->class_class);
    pone_obj_set_ivar_noinc(universe, (pone_val*)obj, "$!name", pone_str_new(universe, name, name_len));
    pone_obj_set_ivar_noinc(universe, (pone_val*)obj, "$!methods", pone_hash_new(universe));
    pone_obj_set_ivar_noinc(universe, (pone_val*)obj, "@!parents", pone_ary_new(universe, 0));

    return (pone_val*)obj;
}

void pone_class_push_parent(pone_universe* universe, pone_val* obj, pone_val* klass) {
    pone_val* parents = pone_obj_get_ivar(universe, obj, "@!parents");
    pone_ary_append(universe, parents, klass);
}

void pone_add_method_c(pone_universe* universe, pone_val* klass, const char* name, size_t name_len, pone_funcptr_t funcptr) {
    assert(klass);
    pone_val* code = pone_code_new_c(universe, funcptr);
    pone_add_method(universe, klass, name, name_len, code);
    pone_refcnt_dec(universe, code);
}

void pone_add_method(pone_universe* universe, pone_val* klass, const char* name, size_t name_len, pone_val* method) {
    assert(klass);
    assert(pone_type(klass) == PONE_OBJ);
    assert(pone_type(method) == PONE_CODE);

    pone_val* methods = pone_obj_get_ivar(universe, klass, "$!methods");
    assert(pone_type(methods) == PONE_HASH);
    pone_hash_put_c(universe, methods, name, name_len, method);
}

static void _compose(pone_universe* universe, pone_val* target_methods, pone_val* klass) {
    pone_val* methods = pone_obj_get_ivar(universe, klass, "$!methods");

    const char* k;
    pone_val* v;
    kh_foreach(methods->as.hash.h, k, v, {
        assert(v);
        if (!pone_hash_exists_c(universe, target_methods, k)) {
            pone_hash_put_c(universe, target_methods, k, strlen(k), v);
        }
    });

    pone_val* parents = pone_obj_get_ivar(universe, klass, "@!parents");
    assert(pone_type(parents) == PONE_ARRAY);
    int l = pone_ary_elems(parents);
    for (int i=0; i<l; ++i) {
        _compose(universe, target_methods, pone_ary_at_pos(parents, i));
    }
}

// .^compose
void pone_class_compose(pone_universe* universe, pone_val* klass) {
    pone_val* methods = pone_obj_get_ivar(universe, klass, "$!methods");
    _compose(universe, methods, klass);
}

pone_val* pone_find_method(pone_world* world, pone_val* obj, const char* name) {
    pone_val* klass = pone_what(world->universe, obj);
    assert(klass);
    pone_val* methods = pone_obj_get_ivar(world->universe, klass, "$!methods");
    assert(methods);
    pone_val* method = pone_hash_at_pos_c(world->universe, methods, name);
    assert(method);
    if (pone_defined(method)) {
        return method;
    } else {
        return pone_nil();
    }
}

// Usage: return pone_call_method(world, iter, "pull-one", 0);
pone_val* pone_call_method(pone_world* world, pone_val* obj, const char* method_name, int n, ...) {
    assert(obj);

    pone_val* method = pone_find_method(world, obj, method_name);
    if (pone_defined(method)) {
        va_list args;

        va_start(args, n);

        pone_val* retval = pone_code_vcall(world, method, obj, n, args);

        va_end(args);
        return retval;
    } else {
        pone_throw_str(world, "Method '%s' not found for invocant of class '%s'", method_name, pone_what_str_c(obj));
    }
}

pone_val* pone_call_meta_method(pone_world* world, pone_val* obj, const char* method_name, int n, ...) {
    if (strcmp(method_name, "methods") == 0) {
        // .^methods
        pone_val* klass = pone_what(world->universe, obj);
        pone_val* methods = pone_obj_get_ivar(world->universe, klass, "$!methods");
        return pone_hash_keys(world, methods);
    } else {
        pone_throw_str(world, "Meta method '%s' not found for invocant of class '%s'", method_name, pone_what_str_c(obj));
    }
}

