
pone_val* pone_new_ary(pone_world* world, int n, ...) {
    va_list list;

    pone_ary* av = (pone_ary*)pone_obj_alloc(world, PONE_ARRAY);

    va_start(list, n);
    av->a = (pone_val**)pone_malloc(world, sizeof(pone_ary)*n);
    av->max = n;
    av->len = n;
    // we can optimize in case of `[1,2,3]`.
    for (int i=0; i<n; ++i) {
        pone_val* v = va_arg(list, pone_val*);
        av->a[i] = v;
        pone_refcnt_inc(world, v);
    }
    va_end(list);

    return (pone_val*)av;
}

void pone_ary_free(pone_world* world, pone_val* val) {
    pone_ary* a=(pone_ary*)val;
    size_t l = pone_ary_elems(val);
    for (size_t i=0; i<l; ++i) {
        pone_refcnt_dec(world, a->a[i]);
    }
    pone_free(world, a->a);
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
