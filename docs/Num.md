# NAME

Num - Num literal

# METHODS

## `Num#floor()`

    say 3.14.floor(); # => 3

\*/

static pone\_val\* meth\_num\_floor(pone\_world\* world, pone\_val\* self, int n, va\_list args) {
    assert(n==0);
    assert(pone\_type(self) == PONE\_NUM);

    double num = self->as.num.n;
    if (num > INT_MAX) {
        pone_throw_str(world, "integer overflow");
    }
    return pone_int_new(world->universe, (int)floor(num));
}

void pone\_num\_init(pone\_universe\* universe) {
    assert(universe->class\_num == NULL);

    universe->class_num = pone_class_new(universe, "Num", strlen("Num"));
    pone_add_method_c(universe, universe->class_num, "floor", strlen("floor"), meth_num_floor);
}
