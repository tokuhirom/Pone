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

/**
 * Create new instance of class
 */
pone_val* pone_class_new(pone_universe* universe, const char* name, size_t name_len) {
    pone_val* obj = pone_obj_new(universe, universe->class_class);
    pone_obj_set_ivar_noinc(universe, (pone_val*)obj, "$!name", pone_str_new(universe, name, name_len));
    pone_obj_set_ivar_noinc(universe, (pone_val*)obj, "$!methods", pone_hash_new(universe, 0));

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
    assert(pone_type(obj) == PONE_OBJ);

    pone_val* methods = pone_obj_get_ivar(world->universe, obj->as.obj.klass, "$!methods");
    assert(methods);
    pone_val* method = pone_hash_at_pos_c(world->universe, methods, name);
    assert(method);
    if (pone_defined(method)) {
        return method;
    } else {
        // TODO better error message
        pone_die_str(world, "method not found");
    }
}

