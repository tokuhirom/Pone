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
        return universe->class_mu;
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

    return (pone_val*)obj;
}

void pone_add_method_c(pone_universe* universe, pone_val* klass, const char* name, size_t name_len, pone_funcptr_t funcptr) {
    pone_val* code = pone_code_new_c(universe, funcptr);
    pone_add_method(universe, klass, name, name_len, code);
    pone_refcnt_dec(universe, code);
}

void pone_add_method(pone_universe* universe, pone_val* klass, const char* name, size_t name_len, pone_val* method) {
    assert(pone_type(klass) == PONE_OBJ);
    assert(pone_type(method) == PONE_CODE);

    pone_val* methods = pone_obj_get_ivar(universe, klass, "$!methods");
    assert(pone_type(methods) == PONE_HASH);
    pone_hash_put_c(universe, methods, name, name_len, method);
}

pone_val* pone_find_method(pone_world* world, pone_val* obj, const char* name) {
    pone_val* klass = pone_what(world->universe, obj);
    pone_val* methods = pone_obj_get_ivar(world->universe, klass, "$!methods");
    assert(methods);
    return pone_hash_at_pos_c(world->universe, methods, name);
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
        pone_throw_str(world, "method '%s' does not found in %s", method_name, pone_what_str_c(obj));
    }
}

