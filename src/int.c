#include "pone.h" /* PONE_INC */

inline pone_int_t pone_int_val(pone_val* val) {
    assert(pone_type(val) == PONE_INT);
    return ((pone_int*)val)->i;
}

pone_val* pone_int_new(pone_universe* universe, pone_int_t i) {
    pone_int* iv = (pone_int*)pone_obj_alloc(universe, PONE_INT);
    iv->i = i;
    return (pone_val*)iv;
}

// ++$i
pone_val* pone_int_incr(pone_world* world, pone_val* i) {
    if (pone_is_frozen(i)) {
        // TODO should we use better interface?
        // show line number
        fprintf(stderr, "[ERROR] You can't modify an itenger literal: %d", i);
        exit(1);
    }
    ((pone_int*)i)->i++;
}

static pone_val* meth_int_is_prime(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n==0);

    int num = pone_intify(world, self);

    if (num <= 1) return pone_false();
    if (num == 2) return pone_true();
    if (num % 2 == 0) return pone_false();
    for(int i = 3; i < num / 2; i+= 2) {
        if (num % i == 0) {
            return pone_false();
        }
    }
    return pone_true();
}

static pone_val* meth_int_str(pone_world* world, pone_val* self, int n, va_list args) {
    return pone_str_from_int(world->universe, pone_int_val(self));
}

static pone_val* meth_int_int(pone_world* world, pone_val* self, int n, va_list args) {
    return pone_int_new(world->universe, pone_int_val(self));
}

static pone_val* meth_int_accepts(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);

    pone_val* rhs = va_arg(args, pone_val*);
    bool b = pone_eq(world, self, rhs);

    return b ? pone_true() : pone_false();
}

static pone_val* meth_int_num(pone_world* world, pone_val* self, int n, va_list args) {
    return pone_num_new(world->universe, pone_int_val(self));
}

void pone_int_init(pone_universe* universe) {
    assert(universe->class_int == NULL);

    universe->class_int = pone_class_new(universe, "Int", strlen("Int"));
    pone_class_push_parent(universe, universe->class_int, universe->class_cool);
    pone_add_method_c(universe, universe->class_int, "is-prime", strlen("is-prime"), meth_int_is_prime);
    pone_add_method_c(universe, universe->class_int, "Str", strlen("Str"), meth_int_str);
    pone_add_method_c(universe, universe->class_int, "Int", strlen("Int"), meth_int_int);
    pone_add_method_c(universe, universe->class_int, "Num", strlen("Num"), meth_int_num);
    pone_add_method_c(universe, universe->class_int, "ACCEPTS", strlen("ACCEPTS"), meth_int_accepts);

    pone_class_compose(universe, universe->class_int);
}

