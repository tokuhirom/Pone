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

int pone_ary_elems(pone_val* av) {
    assert(pone_type(av) == PONE_ARRAY);
    return ((pone_ary*)av)->len;
}

// create new iterator
pone_val* pone_ary_iter_new(pone_universe* universe, pone_val* val) {
    pone_val*iter = pone_obj_new(universe, universe->class_ary_iter);
    pone_obj_set_ivar(universe, iter, "$!i", pone_new_int(universe, 0));
    pone_obj_set_ivar(universe, iter, "$!val", val);

    return iter;
}

static pone_val* meth_iter_next(pone_world* world, int n, va_list args) {
    assert(n == 1);

    pone_val* obj = va_arg(args, pone_val*);
    assert(pone_type(obj) == PONE_OBJ);
    pone_val* ary = pone_obj_get_ivar(world->universe, obj, "$!val");
    pone_val* i = pone_obj_get_ivar(world->universe, obj, "$!i");
    assert(pone_type(i) == PONE_INT);

    if (pone_int_val(i) != pone_ary_elems(ary)) {
        pone_val* val = pone_ary_at_pos(ary, pone_int_val(i));
        pone_int_incr(world, i);
        return val;
    } else {
        pone_die(world, pone_obj_alloc(world->universe, PONE_CONTROL_BREAK));
    }
}

void pone_ary_iter_init(pone_universe* universe) {
    assert(universe->class_ary_iter == NULL);

    pone_val* klass = pone_class_new(universe, "Array::Iterator", strlen("Array::Iterator"));
    pone_add_method_c(universe, klass, "ITER-NEXT", strlen("ITER-NEXT"), meth_iter_next);
    assert(pone_type(klass) == PONE_CLASS);
    universe->class_ary_iter = klass;
}


