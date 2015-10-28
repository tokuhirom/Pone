#include "pone.h" /* PONE_INC */

pone_val* pone_new_ary(pone_universe* universe, int n, ...) {
    va_list list;

    pone_ary* av = (pone_ary*)pone_obj_alloc(universe, PONE_ARRAY);

    va_start(list, n);
    av->a = (pone_val**)pone_malloc(universe, sizeof(pone_ary)*n);
    av->max = n;
    av->len = n;
    // we can optimize in case of `[1,2,3]`.
    for (int i=0; i<n; ++i) {
        pone_val* v = va_arg(list, pone_val*);
        av->a[i] = v;
        pone_refcnt_inc(universe, v);
    }
    va_end(list);

    return (pone_val*)av;
}

void pone_ary_free(pone_universe* universe, pone_val* val) {
    pone_ary* a=(pone_ary*)val;
    size_t l = pone_ary_elems(val);
    for (size_t i=0; i<l; ++i) {
        pone_refcnt_dec(universe, a->a[i]);
    }
    pone_free(universe, a->a);
}

pone_val* pone_ary_at_pos(pone_val* av, int i) {
    assert(pone_type(av) == PONE_ARRAY);
    pone_ary*a = (pone_ary*)av;
    if (a->len > i) {
        return a->a[i];
    } else {
        return pone_nil();
    }
}

size_t pone_ary_elems(pone_val* av) {
    assert(pone_type(av) == PONE_ARRAY);
    return ((pone_ary*)av)->len;
}

// create new iterator
pone_val* pone_ary_iter_new(pone_universe* universe, pone_val* val) {
    pone_refcnt_inc(universe, val);

    pone_ary_iter* iter = (pone_ary_iter*)pone_obj_alloc(universe, PONE_ARRAY_ITER);
    iter->val = val;
    iter->idx = 0;

    return (pone_val*)iter;
}

pone_val* pone_ary_iter_free(pone_universe* universe, pone_val* val) {
    assert(pone_type(val) == PONE_ARRAY_ITER);
    pone_refcnt_dec(universe, val->as.ary_iter.val);
}

