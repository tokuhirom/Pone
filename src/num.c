#include "pone.h" /* PONE_INC */
#include <math.h>

/*

=head1 NAME

Num - Num literal

=head1 METHODS

=cut

*/

pone_val* pone_num_new(pone_world* world, pone_num_t n) {
    pone_num* nv = (pone_num*)pone_obj_alloc(world, PONE_NUM);
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

PONE_FUNC(meth_num_floor) {
    PONE_ARG("Num#floor", "");

    double num = self->as.num.n;
    if (num > INT_MAX) {
        pone_throw_str(world, "integer overflow");
    }
    return pone_int_new(world, (int)floor(num));
}

PONE_FUNC(meth_num_str) {
    PONE_ARG("Num#Str", "");
    return pone_str_from_num(world, pone_num_val(self));
}

PONE_FUNC(meth_num_num) {
    PONE_ARG("Num#Num", "");
    return pone_num_new(world, pone_num_val(self));
}

PONE_FUNC(meth_num_int) {
    PONE_ARG("Num#Int", "");
    return pone_int_new(world, (pone_int_t)pone_num_val(self));
}

void pone_num_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_num == NULL);

    universe->class_num = pone_class_new(world, "Num", strlen("Num"));
    pone_add_method_c(world, universe->class_num, "floor", strlen("floor"), meth_num_floor);
    pone_add_method_c(world, universe->class_num, "Str", strlen("Str"), meth_num_str);
    pone_add_method_c(world, universe->class_num, "Num", strlen("Num"), meth_num_num);
    pone_add_method_c(world, universe->class_num, "Int", strlen("Int"), meth_num_int);
    pone_class_compose(world, universe->class_num);
}
