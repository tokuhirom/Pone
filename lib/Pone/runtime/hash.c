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

void pone_hash_put_c(pone_universe* universe, pone_val* hv, const char* key, int key_len, pone_val* v) {
    assert(pone_type(hv) == PONE_HASH);
    int ret;
    const char* ks=pone_strdup(universe, key, key_len);
    khint_t k = kh_put(str, ((pone_hash*)hv)->h, ks, &ret);
    if (ret == -1) {
        abort(); // TODO better error msg
    }
    kh_val(((pone_hash*)hv)->h, k) = v;
    pone_refcnt_inc(universe, v);
    ((pone_hash*)hv)->len++;
}

pone_val* pone_hash_at_pos_c(pone_universe* universe, pone_val* hash, const char* name) {
    assert(pone_type(hash) == PONE_HASH);
    assert(hash->as.hash.h);

    khint_t k = kh_get(str, hash->as.hash.h, name);
    if (k != kh_end(hash->as.hash.h)) {
        return kh_val(hash->as.hash.h, k);
    } else {
        return pone_nil();
    }
}

void pone_hash_put(pone_universe* universe, pone_val* hv, pone_val* k, pone_val* v) {
    k = pone_to_str(universe, k);
    pone_hash_put_c(universe, hv, pone_string_ptr(k), pone_string_len(k), v);
    pone_refcnt_dec(universe, k);
}

size_t pone_hash_elems(pone_val* val) {
    assert(pone_type(val) == PONE_HASH);
    return ((pone_hash*)val)->len;
}

