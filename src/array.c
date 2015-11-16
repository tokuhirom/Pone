#include "pone.h" /* PONE_INC */

void pone_ary_mark(pone_val* val) {
    pone_int_t l = val->as.ary.len;
    for (pone_int_t i=0; i<l; i++) {
        pone_gc_mark_value(val->as.ary.a[i]);
    }
}

pone_val* pone_ary_new(pone_world* world, pone_int_t n, ...) {
    va_list list;

    GC_LOCK(world->universe);
    pone_ary* av = (pone_ary*)pone_obj_alloc(world, PONE_ARRAY);

    va_start(list, n);
    av->a = (pone_val**)pone_malloc(world->universe, sizeof(pone_val)*n);
    if (!av->a) {
        abort();
    }
    av->max = n;
    av->len = n;
    // we can optimize in case of `[1,2,3]`.
    for (pone_int_t i=0; i<n; ++i) {
        pone_val* v = va_arg(list, pone_val*);
        av->a[i] = v;
    }
    GC_UNLOCK(world->universe);
    va_end(list);

    return (pone_val*)av;
}

void pone_ary_free(pone_universe* universe, pone_val* val) {
    pone_free(universe, val->as.ary.a);
}

// $av.AT-POS(i)
// $av[i]
pone_val* pone_ary_at_pos(pone_val* av, pone_int_t i) {
    assert(pone_type(av) == PONE_ARRAY);
    pone_ary*a = (pone_ary*)av;
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
    GC_LOCK(universe);

    if (self->as.ary.len < size) {
        if (self->as.ary.max < size) { // need realloc
            self->as.ary.max = size;
            self->as.ary.a = realloc(self->as.ary.a, sizeof(pone_val*)*self->as.ary.max);
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

    GC_UNLOCK(universe);
}

void pone_ary_assign_pos(pone_world* world, pone_val* self, pone_val* pos, pone_val* val) {
    GC_LOCK(world->universe);

    assert(self); assert(pos); assert(val);
    pone_universe* universe = world->universe;

    assert(pone_type(self) == PONE_ARRAY);
    pone_ary*a = (pone_ary*)self;
    pone_int_t i = pone_intify(world, pos);

    if (a->len > i) {
        a->a[i] = val;
    } else {
        pone_ary_resize(universe, self, i+1);
        self->as.ary.a[i] = val;
    }
    GC_UNLOCK(world->universe);
}

// this method is *not* thread safe.
static pone_val* meth_pull_one(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);

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

/*

=head1 NAME

Array - Built-in Array class.

=head1 LITERAL

There is a literal to build an instance.

    [1,2,3]

=head1 METHODS

=cut

*/

/*

=head3 C<Array#iterator()>

    my $iter = [1,2,3].iterator();

Get new instance of an iterator.

=cut

*/
static pone_val* meth_ary_iterator(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);

    assert(pone_type(self) == PONE_ARRAY);

    // self!iterator-class.bless(i => 0, val => self)
    pone_val* iterator_class = pone_obj_get_ivar(world, pone_what(world, self), "$!iterator-class");
    pone_val* iter = pone_obj_new(world, iterator_class);
    pone_obj_set_ivar(world, iter, "$!i", pone_int_new(world, 0));
    pone_obj_set_ivar(world, iter, "$!val", self);
    return iter;
}

/*

=head2 C<Array#elems() --> Int>

Get the number of elements.

=cut

*/
static pone_val* meth_ary_elems(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);
    assert(pone_type(self) == PONE_ARRAY);
    return pone_int_new(world, pone_ary_elems(self));
}

void pone_ary_append(pone_universe* universe, pone_val* self, pone_val* val) {
    GC_LOCK(universe);

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
    GC_UNLOCK(universe);
}

/*

=head2 C<Array#append($val)>

    my $a = [1,2,3];
    $a.append(4);
    say $a.join(","); # => 1,2,3,4

append an element to array tail.

=cut

*/
static pone_val* meth_ary_append(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);

    pone_val* val = va_arg(args, pone_val*);
    pone_ary_append(world->universe, self, val);

    return pone_nil();
}

pone_val* pone_ary_last(pone_world* world, pone_val* self) {
    assert(pone_type(self) == PONE_ARRAY);
    if (self->as.ary.len == 0) {
        pone_throw_str(world, "Cannot get last element from an empty Array");
    }

    return self->as.ary.a[self->as.ary.len-1];
}

pone_val* pone_ary_pop(pone_world* world, pone_val* self) {
    if (self->as.ary.len == 0) {
        pone_throw_str(world, "Cannot pop from an empty Array");
    }

    GC_LOCK(world->universe);
    pone_val* retval = self->as.ary.a[self->as.ary.len-1];
    self->as.ary.a[self->as.ary.len-1] = NULL;
    self->as.ary.len--;
    GC_UNLOCK(world->universe);
    return retval;
}

/*

=head2 C<Array#pop($val)>

    my $a = [1,2,3];
    say $a.pop(); # => 3

Pop element from an array.

=cut

*/
static pone_val* meth_ary_pop(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);
    return pone_ary_pop(world, self);
}

/*

=head2 C<Array#prepend($val)>

    my $a = [1,2,3];
    $a.prepend(0);
    say $a.join(','); # => 0,1,2,3

Prepend element to an array.

NYI

=cut

*/

/*

=head2 C<Array#shift($val)>

    my $a = [1,2,3];
    say $a.shift(); # => 1

Removes and returns the first item from the array. Fails for an empty arrays.

NYI

=cut

*/

/*

=head2 C<Array#end()>

Returns the index of the last element.

NYI


=cut

*/

/*

=head2 C<Array#join($separator)>

    say [1,2,3].join(','); # 1,2,3

Treats the elements of the list as strings, interleaves them with $separator and concatenates everything into a single string.

NYI

=cut

*/

static pone_val* meth_ary_str(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);

    pone_val* v = pone_str_new(world, "", 0);
    pone_str_append_c(world, v, "(", 1);
    for (pone_int_t i=0; i<pone_ary_elems(self); ++i) {
        pone_str_append(world, v, pone_ary_at_pos(self, i));
        pone_str_append_c(world, v, " ", 1);
    }
    pone_str_append_c(world, v, ")", 1);
    return v;
}

/*

=head2 C<Array#ASSIGN-POS($pos, $value)>

    $ary.ASSIGN-POS($pos, $value);

is equivalent to

    $ary[$pos] = $value;

=cut

*/
static pone_val* meth_ary_assign_pos(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 2);

    pone_val*pos = va_arg(args, pone_val*);
    pone_val*value = va_arg(args, pone_val*);

    pone_ary_assign_pos(world, self, pos, value);
    return value;
}

void pone_ary_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_ary == NULL);

    pone_val* iter_class = pone_class_new(world, "Array::Iterator", strlen("Array::Iterator"));
    pone_add_method_c(world, iter_class, "pull-one", strlen("pull-one"), meth_pull_one);

    universe->class_ary = pone_class_new(world, "Array", strlen("Array"));
    pone_class_push_parent(world, universe->class_ary, universe->class_any);
    pone_add_method_c(world, universe->class_ary, "iterator", strlen("iterator"), meth_ary_iterator);
    pone_add_method_c(world, universe->class_ary, "elems", strlen("elems"), meth_ary_elems);
    pone_add_method_c(world, universe->class_ary, "append", strlen("append"), meth_ary_append);
    pone_add_method_c(world, universe->class_ary, "pop", strlen("pop"), meth_ary_pop);
    pone_add_method_c(world, universe->class_ary, "Str", strlen("Str"), meth_ary_str);
    pone_add_method_c(world, universe->class_ary, "ASSIGN-POS", strlen("ASSIGN-POS"), meth_ary_assign_pos);
    pone_obj_set_ivar(world, universe->class_ary, "$!iterator-class", iter_class);

    pone_class_compose(world, universe->class_ary);
}

