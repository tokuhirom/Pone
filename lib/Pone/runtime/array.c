#include "pone.h" /* PONE_INC */

pone_val* pone_ary_new(pone_universe* universe, int n, ...) {
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

static pone_val* meth_pull_one(pone_world* world, int n, va_list args) {
    assert(n == 1);

    pone_val* obj = va_arg(args, pone_val*);
    assert(pone_type(obj) == PONE_OBJ);
    pone_val* ary = pone_obj_get_ivar(world->universe, obj, "$!val");
    pone_val* i = pone_obj_get_ivar(world->universe, obj, "$!i");
    assert(pone_type(i) == PONE_INT);

    if (pone_int_val(i) != pone_ary_elems(ary)) {
        pone_val* val = pone_ary_at_pos(ary, pone_int_val(i));
        pone_refcnt_inc(world->universe, val);
        pone_int_incr(world, i);
        return val;
    } else {
        pone_refcnt_inc(world->universe, world->universe->instance_iteration_end);
        return world->universe->instance_iteration_end;
    }
}

static pone_val* meth_ary_elems(pone_world* world, int n, va_list args) {
    assert(n == 1);
    pone_val* obj = va_arg(args, pone_val*);
    assert(pone_type(obj) == PONE_ARRAY);
    return pone_int_new(world->universe, pone_ary_elems(obj));
}

// Array#iterator
static pone_val* meth_ary_iterator(pone_world* world, int n, va_list args) {
    assert(n == 1);

    pone_val* self = va_arg(args, pone_val*);
    assert(pone_type(self) == PONE_ARRAY);

    // self!iterator-class.bless(i => 0, val => self)
    pone_val* iterator_class = pone_obj_get_ivar(world->universe, pone_what(world->universe, self), "$!iterator-class");
    pone_val* iter = pone_obj_new(world->universe, iterator_class);
    pone_obj_set_ivar_noinc(world->universe, iter, "$!i", pone_int_new(world->universe, 0));
    pone_obj_set_ivar(world->universe, iter, "$!val", self);
    return iter;
}

void pone_ary_init(pone_universe* universe) {
    assert(universe->class_ary == NULL);

    pone_val* iter_class = pone_class_new(universe, "Array::Iterator", strlen("Array::Iterator"));
    pone_add_method_c(universe, iter_class, "pull-one", strlen("pull-one"), meth_pull_one);

    universe->class_ary = pone_class_new(universe, "Array", strlen("Array"));
    pone_add_method_c(universe, universe->class_ary, "iterator", strlen("iterator"), meth_ary_iterator);
    pone_obj_set_ivar_noinc(universe, universe->class_ary, "$!iterator-class", iter_class);

}

