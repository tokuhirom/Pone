#include "pone.h" /* PONE_INC */

// TODO: delete key
// TODO: push key
// TODO: exists key

pone_val* pone_new_hash(pone_universe* universe, int n, ...) {
    va_list list;

    pone_hash* hv = (pone_hash*)pone_obj_alloc(universe, PONE_HASH);
    hv->h = kh_init(str);

    va_start(list, n);
    // we can optimize in case of `{a => 3}`. we can omit mortalize.
    for (int i=0; i<n; i+=2) {
        pone_val* k = va_arg(list, pone_val*);
        pone_val* v = va_arg(list, pone_val*);
        pone_hash_put(universe, (pone_val*)hv, k, v);
    }
    va_end(list);

    return (pone_val*)hv;
}

void pone_hash_free(pone_universe* universe, pone_val* val) {
    pone_hash* h=(pone_hash*)val;
    const char* k;
    pone_val* v;
    kh_foreach(h->h, k, v, {
        pone_free(universe, (void*)k); // k is strdupped.
        pone_refcnt_dec(universe, v);
    });
    kh_destroy(str, h->h);
}

void pone_hash_put(pone_universe* universe, pone_val* hv, pone_val* k, pone_val* v) {
    assert(pone_type(hv) == PONE_HASH);
    k = pone_to_str(universe, k);
    // TODO: check ret
    int ret;
    const char* ks=pone_strdup(universe, pone_string_ptr(k), pone_string_len(k));
    khint_t key = kh_put(str, ((pone_hash*)hv)->h, ks, &ret);
    kh_val(((pone_hash*)hv)->h, key) = v;
    pone_refcnt_inc(universe, v);
    ((pone_hash*)hv)->len++;
    pone_refcnt_dec(universe, k);
}

size_t pone_hash_elems(pone_val* val) {
    assert(pone_type(val) == PONE_HASH);
    return ((pone_hash*)val)->len;
}

