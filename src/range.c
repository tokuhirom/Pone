#include "pone.h"

/*

=head1 NAME

Range - 1..10

=head1 METHODS

=cut

*/

pone_val* pone_range_new(pone_world* world, pone_val* min, pone_val* max) {
    assert(min);
    assert(max);

    pone_val* obj = pone_obj_new(world->universe, world->universe->class_range);
    pone_obj_set_ivar(world->universe, obj, "$!min", min);
    pone_obj_set_ivar(world->universe, obj, "$!max", max);
    return obj;
}

/*

=head2 C<Range#min>

=cut

*/
static pone_val* meth_range_min(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);

    return pone_obj_get_ivar(world->universe, world->universe->class_range, "$!min");
}

/*

=head2 C<Range#Str>

Get a string notation of Range object.

=cut

 */
static pone_val* meth_range_str(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);

    pone_val* min = pone_obj_get_ivar(world->universe, self, "$!min");
    pone_val* max = pone_obj_get_ivar(world->universe, self, "$!max");

    pone_val* str = pone_stringify(world, min);
    pone_str_append_c(world, str, "..", strlen(".."));
    pone_str_append(world, str, max);
    return str;
}

/*

=head2 C<Range#max>

=cut

*/
static pone_val* meth_range_max(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);

    return pone_obj_get_ivar(world->universe, world->universe->class_range, "$!max");
}

static pone_val* meth_range_iterator(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);

    assert(pone_type(self) == PONE_OBJ);

    pone_val* min = pone_obj_get_ivar(world->universe, self, "$!min");
    pone_val* max = pone_obj_get_ivar(world->universe, self, "$!max");

    // self!iterator-class.bless(i => self.min, max => self.max)
    pone_val* iterator_class = pone_obj_get_ivar(world->universe, pone_what(world->universe, self), "$!iterator-class");
    pone_val* iter = pone_obj_new(world->universe, iterator_class);
    pone_obj_set_ivar_noinc(world->universe, iter, "$!i", pone_int_new(world->universe, pone_intify(world, min)));
    pone_obj_set_ivar(world->universe, iter, "$!max", max);
    return iter;
}

static pone_val* meth_pull_one(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);
    assert(pone_type(self) == PONE_OBJ);

    pone_val* i_val = pone_obj_get_ivar(world->universe, self, "$!i");
    pone_val* max_val = pone_obj_get_ivar(world->universe, self, "$!max");
    pone_int_t i = pone_intify(world, i_val);
    pone_int_t max = pone_intify(world, max_val);

    if (i > max) {
        return world->universe->instance_iteration_end;
    } else {
        pone_val* retval = pone_int_new(world->universe, pone_intify(world, i_val));
        pone_int_incr(world, i_val);
        return retval;
    }
}

void pone_range_init(pone_universe* universe) {
    assert(universe->class_range == NULL);

    pone_val* iter_class = pone_class_new(universe, "Range::Iterator", strlen("Range::Iterator"));
    pone_add_method_c(universe, iter_class, "pull-one", strlen("pull-one"), meth_pull_one);

    universe->class_range = pone_class_new(universe, "Range", strlen("Range"));
    pone_add_method_c(universe, universe->class_range, "iterator", strlen("iterator"), meth_range_iterator);
    pone_add_method_c(universe, universe->class_range, "min", strlen("min"), meth_range_min);
    pone_add_method_c(universe, universe->class_range, "max", strlen("max"), meth_range_max);
    pone_add_method_c(universe, universe->class_range, "Str", strlen("Str"), meth_range_str);
    pone_obj_set_ivar_noinc(universe, universe->class_range, "$!iterator-class", iter_class);

    pone_class_compose(universe, universe->class_range);

    pone_universe_set_global(universe, "Range", universe->class_range);
}
