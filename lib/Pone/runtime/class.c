#include "pone.h" /* PONE_INC */

/**
 * Class related operations
 */

/**
 * Create new instance of class
 */
pone_val* pone_class_new(pone_universe* universe, const char* name, size_t name_len) {
    pone_class* obj = (pone_class*)pone_obj_alloc(universe, PONE_CLASS);
    obj->name = pone_strdup(universe, name, name_len);
    obj->methods = kh_init(str);

    return (pone_val*)obj;
}

pone_val* pone_class_free(pone_universe* universe, pone_val* val) {
    assert(pone_type(val) == PONE_CLASS);

    // free instance variables
    const char* k;
    pone_val* v;
    kh_foreach(val->as.klass.methods, k, v, {
        pone_free(universe, (void*)k); // k is strdupped.
        pone_refcnt_dec(universe, v);
    });

    kh_destroy(str, val->as.klass.methods);

    pone_free(universe, val->as.klass.name);
}

pone_val* pone_add_method_c(pone_universe* universe, pone_val* klass, const char* name, size_t name_len, pone_funcptr_t funcptr) {
    pone_val* code = pone_code_new_c(universe, funcptr);
    pone_add_method(universe, klass, name, name_len, code);
    pone_refcnt_dec(universe, code);
}

pone_val* pone_add_method(pone_universe* universe, pone_val* klass, const char* name, size_t name_len, pone_val* method) {
    assert(pone_type(klass) == PONE_CLASS);
    assert(pone_type(method) == PONE_CODE);

    int ret;
    const char* ks=pone_strdup(universe, name, name_len);
    khint_t key = kh_put(str, klass->as.klass.methods, ks, &ret);
    if (ret == -1) {
        fprintf(stderr, "[ERROR] kh_put error\n");
    }
    kh_val(klass->as.klass.methods, key) = method;
    pone_refcnt_inc(universe, method);
}

pone_val* pone_find_method(pone_world* world, pone_val* obj, const char* name) {
    assert(pone_type(obj) == PONE_OBJ);

    khint_t entry = kh_get(str, obj->as.obj.klass->as.klass.methods, name);
    if (kh_end(obj->as.obj.klass->as.klass.methods) != entry) {
        return kh_val(obj->as.obj.klass->as.klass.methods, entry);
    } else {
        // TODO better error message
        pone_die_str(world, "method not found");
    }
}

