#include "pone.h" /* PONE_INC */

// TODO: delete key
// TODO: push key
// TODO: exists key

pone_val* pone_hash_new(pone_universe* universe) {
    pone_val* hv = pone_obj_alloc(universe, PONE_HASH);
    hv->as.hash.h = kh_init(str);
    return hv;
}

pone_val* pone_hash_puts(pone_world* world, pone_val* hash, int n, ...) {
    va_list list;

    va_start(list, n);
    // we can optimize in case of `{a => 3}`. we can omit mortalize.
    for (int i=0; i<n; i+=2) {
        pone_val* k = va_arg(list, pone_val*);
        pone_val* v = va_arg(list, pone_val*);
        pone_hash_put(world, hash, k, v);
    }
    va_end(list);

    return hash; // return itself
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

void pone_hash_put(pone_world* world, pone_val* hv, pone_val* k, pone_val* v) {
    k = pone_stringify(world->universe, k);
    pone_hash_put_c(world->universe, hv, pone_str_ptr(k), pone_str_len(k), v);
    pone_refcnt_dec(world->universe, k);
}

size_t pone_hash_elems(pone_val* val) {
    assert(pone_type(val) == PONE_HASH);
    return ((pone_hash*)val)->len;
}

/*

=head2 C<Hash#elems() --> Int>

Get the number of elements.

=cut

*/
static pone_val* meth_hash_elems(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);
    return pone_int_new(world->universe, pone_hash_elems(self));
}

void pone_hash_init(pone_universe* universe) {
    assert(universe->class_hash == NULL);

    universe->class_hash = pone_class_new(universe, "Hash", strlen("Hash"));
    pone_add_method_c(universe, universe->class_hash, "elems", strlen("elems"), meth_hash_elems);
}

