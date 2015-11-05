#include "pone.h" /* PONE_INC */

pone_val* pone_ary_new(pone_universe* universe, int n, ...) {
    va_list list;

    pone_ary* av = (pone_ary*)pone_obj_alloc(universe, PONE_ARRAY);

    va_start(list, n);
    av->a = (pone_val**)pone_malloc(universe, sizeof(pone_val)*n);
    if (!av->a) {
        abort();
    }
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

static pone_val* meth_pull_one(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);

    assert(pone_type(self) == PONE_OBJ);
    pone_val* ary = pone_obj_get_ivar(world->universe, self, "$!val");
    pone_val* i = pone_obj_get_ivar(world->universe, self, "$!i");
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
    pone_val* iterator_class = pone_obj_get_ivar(world->universe, pone_what(world->universe, self), "$!iterator-class");
    pone_val* iter = pone_obj_new(world->universe, iterator_class);
    pone_obj_set_ivar_noinc(world->universe, iter, "$!i", pone_int_new(world->universe, 0));
    pone_obj_set_ivar(world->universe, iter, "$!val", self);
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
    return pone_int_new(world->universe, pone_ary_elems(self));
}

void pone_ary_append(pone_universe* universe, pone_val* self, pone_val* val) {
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
    pone_refcnt_inc(universe, val);
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

/*

=head2 C<Array#pop($val)>

    my $a = [1,2,3];
    say $a.pop(); # => 3

Pop element from an array.

=cut

*/
static pone_val* meth_ary_pop(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);

    if (self->as.ary.len == 0) {
        pone_throw_str(world, "Cannot pop from an empty Array");
    }

    pone_val* retval = self->as.ary.a[self->as.ary.len-1];
    self->as.ary.a[self->as.ary.len-1] = NULL;
    self->as.ary.len--;

    return retval;
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

    pone_val* v = pone_str_new(world->universe, "", 0);
    pone_str_append_c(world, v, "(", 1);
    for (int i=0; i<pone_ary_elems(self); ++i) {
        pone_str_append(world, v, pone_ary_at_pos(self, i));
        pone_str_append_c(world, v, " ", 1);
    }
    pone_str_append_c(world, v, ")", 1);
    return v;
}

void pone_ary_init(pone_universe* universe) {
    assert(universe->class_ary == NULL);

    pone_val* iter_class = pone_class_new(universe, "Array::Iterator", strlen("Array::Iterator"));
    pone_add_method_c(universe, iter_class, "pull-one", strlen("pull-one"), meth_pull_one);

    universe->class_ary = pone_class_new(universe, "Array", strlen("Array"));
    pone_class_push_parent(universe, universe->class_ary, universe->class_any);
    pone_add_method_c(universe, universe->class_ary, "iterator", strlen("iterator"), meth_ary_iterator);
    pone_add_method_c(universe, universe->class_ary, "elems", strlen("elems"), meth_ary_elems);
    pone_add_method_c(universe, universe->class_ary, "append", strlen("append"), meth_ary_append);
    pone_add_method_c(universe, universe->class_ary, "pop", strlen("pop"), meth_ary_pop);
    pone_add_method_c(universe, universe->class_ary, "Str", strlen("Str"), meth_ary_str);
    pone_obj_set_ivar_noinc(universe, universe->class_ary, "$!iterator-class", iter_class);

    pone_class_compose(universe, universe->class_ary);
}

