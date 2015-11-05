#include "pone.h" /* PONE_INC */
#include <math.h>

/*

=head1 NAME

Num - Num literal

=head1 METHODS

=cut

*/

pone_val* pone_num_new(pone_universe* universe, double n) {
    pone_num* nv = (pone_num*)pone_obj_alloc(universe, PONE_NUM);
    nv->n = n;
    return (pone_val*)nv;
}

inline double pone_num_val(pone_val* val) {
    assert(pone_type(val) == PONE_NUM);
    return ((pone_num*)val)->n;
}

/*

=head2 C<Num#floor()>

    say 3.14.floor(); # => 3

*/

static pone_val* meth_num_floor(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n==0);
    assert(pone_type(self) == PONE_NUM);

    double num = self->as.num.n;
    if (num > INT_MAX) {
        pone_throw_str(world, "integer overflow");
    }
    return pone_int_new(world->universe, (int)floor(num));
}

static pone_val* meth_num_str(pone_world* world, pone_val* self, int n, va_list args) {
    return pone_str_from_num(world->universe, pone_num_val(self));
}

static pone_val* meth_num_num(pone_world* world, pone_val* self, int n, va_list args) {
    return pone_num_new(world->universe, pone_num_val(self));
}

void pone_num_init(pone_universe* universe) {
    assert(universe->class_num == NULL);

    universe->class_num = pone_class_new(universe, "Num", strlen("Num"));
    pone_class_push_parent(universe, universe->class_num, universe->class_cool);
    pone_add_method_c(universe, universe->class_num, "floor", strlen("floor"), meth_num_floor);
    pone_add_method_c(universe, universe->class_num, "Str", strlen("Str"), meth_num_str);
    pone_add_method_c(universe, universe->class_num, "Num", strlen("Num"), meth_num_num);
    pone_class_compose(universe, universe->class_num);
}

