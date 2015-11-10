#include "pone.h" /* PONE_INC */

pone_val* pone_obj_new(pone_universe* universe, pone_val* klass) {
    assert(klass);
    assert(pone_type(klass) == PONE_OBJ);

    pone_obj* obj = (pone_obj*)pone_obj_alloc(universe, PONE_OBJ);
    obj->klass = klass;
    obj->ivar = kh_init(str);

    pone_refcnt_inc(universe, klass);

    return (pone_val*)obj;
}

void pone_obj_free(pone_universe* universe, pone_val* val) {
    assert(pone_type(val) == PONE_OBJ);

    if (val->as.obj.klass != NULL) {
        pone_refcnt_dec(universe, val->as.obj.klass);
    }

    // free instance variables
    const char* k;
    pone_val* v;
    kh_foreach(val->as.obj.ivar, k, v, {
        pone_free(universe, (void*)k); // k is strdupped.
        pone_refcnt_dec(universe, v);
    });

    kh_destroy(str, val->as.obj.ivar);
}

// set instance variable to the object without refinc++
void pone_obj_set_ivar_noinc(pone_universe* universe, pone_val* obj, const char* name, pone_val* val) {
    assert(pone_type(obj) == PONE_OBJ);

    char *ks = pone_strdup(universe, name, strlen(name));
    int ret;
    khint_t key = kh_put(str, obj->as.obj.ivar, ks, &ret);
    if (ret == -1) {
        fprintf(stderr, "[ERROR] cannot put value to instance variable\n");
        abort();
    }
    kh_val(obj->as.obj.ivar, key) = val;
}


// set instance variable to the object
void pone_obj_set_ivar(pone_universe* universe, pone_val* obj, const char* name, pone_val* val) {
    pone_obj_set_ivar_noinc(universe, obj, name, val);
    pone_refcnt_inc(universe, val);
}

// get instance variable from the object
pone_val* pone_obj_get_ivar(pone_universe* universe, pone_val* obj, const char* name) {
    assert(pone_type(obj) == PONE_OBJ);

    khint_t entry = kh_get(str, obj->as.obj.ivar, name);
    if (kh_end(obj->as.obj.ivar) != entry) {
        return kh_val(obj->as.obj.ivar, entry);
    } else {
        return pone_nil();
    }
}

