#include "pone.h" /* PONE_INC */

void pone_ary_mark(pone_val* val) {
    pone_int_t l = val->as.ary.len;
    for (pone_int_t i = 0; i < l; i++) {
        pone_gc_mark_value(val->as.ary.a[i]);
    }
}

// deep copy
pone_val* pone_ary_copy(pone_world* world, pone_val* obj) {
    pone_val* retval = pone_ary_new(world, 0);
    for (pone_int_t i = 0; i < pone_ary_elems(obj); ++i) {
        pone_ary_push(world->universe, retval, pone_val_copy(world, pone_ary_at_pos(obj, i)));
    }
    return retval;
}

pone_val* pone_ary_new(pone_world* world, pone_int_t n, ...) {
    va_list list;

    pone_ary* av = (pone_ary*)pone_obj_alloc(world, PONE_ARRAY);

    va_start(list, n);
    av->a = (pone_val**)pone_malloc(world->universe, sizeof(pone_val) * n);
    if (!av->a) {
        abort();
    }
    av->max = n;
    av->len = n;
    // we can optimize in case of `[1,2,3]`.
    for (pone_int_t i = 0; i < n; ++i) {
        pone_val* v = va_arg(list, pone_val*);
        av->a[i] = v;
    }

    va_end(list);

    return (pone_val*)av;
}

void pone_ary_free(pone_world* world, pone_val* val) {
    pone_free(world->universe, val->as.ary.a);
}

// $av.AT-POS(i)
// $av[i]
pone_val* pone_ary_at_pos(pone_val* av, pone_int_t i) {
    assert(pone_type(av) == PONE_ARRAY);
    pone_ary* a = (pone_ary*)av;
    if (a->len > i) {
        return a->a[i];
    } else {
        return pone_nil();
    }
}

pone_int_t pone_ary_elems(pone_val* av) {
    assert(pone_type(av) == PONE_ARRAY);
    return ((pone_ary*)av)->len;
}

void pone_ary_resize(pone_universe* universe, pone_val* self, pone_int_t size) {

    if (self->as.ary.len < size) {
        if (self->as.ary.max < size) { // need realloc
            self->as.ary.max = size;
            self->as.ary.a = realloc(self->as.ary.a, sizeof(pone_val*) * self->as.ary.max);
            if (!self->as.ary.a) {
                fprintf(stderr, "cannot allocate memory for array\n");
                abort();
            }
        }

        // fill nil
        while (self->as.ary.len < size) {
            self->as.ary.a[self->as.ary.len] = pone_nil();
            self->as.ary.len++;
        }
    } else {
        abort();
    }
}

void pone_ary_assign_pos(pone_world* world, pone_val* self, pone_val* pos, pone_val* val) {
    assert(self);
    assert(pos);
    assert(val);
    pone_universe* universe = world->universe;

    assert(pone_type(self) == PONE_ARRAY);
    pone_ary* a = (pone_ary*)self;
    pone_int_t i = pone_intify(world, pos);

    if (a->len > i) {
        a->a[i] = val;
    } else {
        pone_ary_resize(universe, self, i + 1);
        self->as.ary.a[i] = val;
    }
}

// this method is *not* thread safe.
PONE_FUNC(meth_pull_one) {
    PONE_ARG("Array::Iterator#pull-one", "");

    assert(pone_type(self) == PONE_OBJ);
    pone_val* ary = pone_obj_get_ivar(world, self, "$!val");
    pone_val* i = pone_obj_get_ivar(world, self, "$!i");
    assert(pone_type(i) == PONE_INT);

    if (pone_int_val(i) != pone_ary_elems(ary)) {
        pone_val* val = pone_ary_at_pos(ary, pone_int_val(i));
        pone_int_incr(world, i);
        return val;
    } else {
        return world->universe->instance_iteration_end;
    }
}

PONE_FUNC(meth_ary_iterator) {
    PONE_ARG("Array#iterator", "");

    // self!iterator-class.bless(i => 0, val => self)
    pone_val* iterator_class = pone_obj_get_ivar(world, pone_what(world, self), "$!iterator-class");
    pone_val* iter = pone_obj_new(world, iterator_class);
    pone_obj_set_ivar(world, iter, "$!i", pone_int_new(world, 0));
    pone_obj_set_ivar(world, iter, "$!val", self);
    return iter;
}

PONE_FUNC(meth_ary_elems) {
    PONE_ARG("Array#elems", "");
    assert(pone_type(self) == PONE_ARRAY);
    return pone_int_new(world, pone_ary_elems(self));
}

void pone_ary_push(pone_universe* universe, pone_val* self, pone_val* val) {
    assert(pone_type(self) == PONE_ARRAY);

    if (self->as.ary.max == self->as.ary.len) {
        if (self->as.ary.max > 0) {
            self->as.ary.max *= 2;
        } else {
            self->as.ary.max = 1;
        }
        self->as.ary.a = realloc(self->as.ary.a, sizeof(pone_val) * self->as.ary.max);
        if (!self->as.ary.a) {
            fprintf(stderr, "cannot allocate memory for array\n");
            abort();
        }
    }

    self->as.ary.a[self->as.ary.len++] = val;
}

PONE_FUNC(meth_ary_push) {
    pone_val* val;
    PONE_ARG("Array#push", "o", &val);

    pone_ary_push(world->universe, self, val);

    return pone_nil();
}

void pone_ary_unshift(pone_world* world, pone_val* self, pone_val* val) {
    if (self->as.ary.max == self->as.ary.len) {
        self->as.ary.max = self->as.ary.max ? self->as.ary.max << 1 : 1;
        self->as.ary.a = realloc(self->as.ary.a, sizeof(pone_val*) * self->as.ary.max);
    }
    memmove(self->as.ary.a + 1, self->as.ary.a, sizeof(pone_val*) * (self->as.ary.len));
    self->as.ary.a[0] = val;
    self->as.ary.len++;
}

PONE_FUNC(meth_ary_unshift) {
    pone_val* val;
    PONE_ARG("Array#unshift", "o", &val);

    pone_ary_unshift(world, self, val);
    return pone_nil();
}

pone_val* pone_ary_last(pone_world* world, pone_val* self) {
    assert(pone_type(self) == PONE_ARRAY);
    if (self->as.ary.len == 0) {
        pone_throw_str(world, "Cannot get last element from an empty Array");
    }

    return self->as.ary.a[self->as.ary.len - 1];
}

pone_val* pone_ary_pop(pone_world* world, pone_val* self) {
    if (self->as.ary.len == 0) {
        pone_throw_str(world, "Cannot pop from an empty Array");
    }

    pone_val* retval = self->as.ary.a[self->as.ary.len - 1];
    self->as.ary.a[self->as.ary.len - 1] = NULL;
    self->as.ary.len--;
    pone_save_tmp(world, retval);

    return retval;
}

pone_val* pone_ary_shift(pone_world* world, pone_val* self) {
    if (self->as.ary.len == 0) {
        pone_throw_str(world, "Cannot shift from an empty Array");
    }

    pone_val* retval = self->as.ary.a[0];
    memmove(self->as.ary.a, self->as.ary.a + 1, sizeof(pone_val*) * (self->as.ary.len - 1));
    self->as.ary.len--;
    pone_save_tmp(world, retval);

    return retval;
}

PONE_FUNC(meth_ary_pop) {
    PONE_ARG("Array#pop", "");
    return pone_ary_pop(world, self);
}

PONE_FUNC(meth_ary_shift) {
    PONE_ARG("Array#shift", "");
    return pone_ary_shift(world, self);
}

PONE_FUNC(meth_ary_join) {
    pone_val* separator;
    PONE_ARG("Array#join", "o", &separator);

    pone_val* v = pone_str_new_strdup(world, "", 0);
    pone_int_t len = pone_ary_elems(self);
    for (pone_int_t i = 0; i < len; ++i) {
        pone_str_append(world, v, pone_ary_at_pos(self, i));
        if (i != len - 1) {
            pone_str_append(world, v, separator);
        }
    }
    return v;
}

PONE_FUNC(meth_ary_str) {
    PONE_ARG("Array#Str", "");

    pone_val* v = pone_str_new_strdup(world, "", 0);
    pone_str_append_c(world, v, "[", 1);
    for (pone_int_t i = 0; i < pone_ary_elems(self); ++i) {
        pone_str_append(world, v, pone_ary_at_pos(self, i));
        pone_str_append_c(world, v, ",", 1);
    }
    pone_str_append_c(world, v, "]", 1);
    return v;
}

PONE_FUNC(meth_ary_assign_pos) {
    pone_val* pos, *value;
    PONE_ARG("Array#ASSIGN-POS", "oo", &pos, &value);

    pone_ary_assign_pos(world, self, pos, value);
    return value;
}

void pone_ary_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_ary == NULL);

    pone_val* iter_class = pone_class_new(world, "Array::Iterator", strlen("Array::Iterator"));
    pone_add_method_c(world, iter_class, "pull-one", strlen("pull-one"), meth_pull_one);

    universe->class_ary = pone_class_new(world, "Array", strlen("Array"));
    pone_add_method_c(world, universe->class_ary, "iterator", strlen("iterator"), meth_ary_iterator);
    pone_add_method_c(world, universe->class_ary, "elems", strlen("elems"), meth_ary_elems);
    pone_add_method_c(world, universe->class_ary, "push", strlen("push"), meth_ary_push);
    pone_add_method_c(world, universe->class_ary, "pop", strlen("pop"), meth_ary_pop);
    pone_add_method_c(world, universe->class_ary, "unshift", strlen("unshift"), meth_ary_unshift);
    pone_add_method_c(world, universe->class_ary, "shift", strlen("shift"), meth_ary_shift);
    pone_add_method_c(world, universe->class_ary, "join", strlen("join"), meth_ary_join);
    pone_add_method_c(world, universe->class_ary, "Str", strlen("Str"), meth_ary_str);
    pone_add_method_c(world, universe->class_ary, "ASSIGN-POS", strlen("ASSIGN-POS"), meth_ary_assign_pos);
    pone_obj_set_ivar(world, universe->class_ary, "$!iterator-class", iter_class);
    pone_class_compose(world, universe->class_ary);
}
