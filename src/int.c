#include "pone.h" /* PONE_INC */

inline pone_int_t pone_int_val(pone_val* val) {
    assert(pone_type(val) == PONE_INT);
    return ((pone_int*)val)->i;
}

pone_val* pone_int_new(pone_world* world, pone_int_t i) {
    pone_int* iv = (pone_int*)pone_obj_alloc(world, PONE_INT);
    iv->i = i;
    return (pone_val*)iv;
}

// ++$i
pone_val* pone_int_incr(pone_world* world, pone_val* i) {
    if (pone_is_frozen(i)) {
        // TODO should we use better interface?
        // show line number
        fprintf(stderr, "[ERROR] You can't modify an itenger literal: %p", i);
        exit(1);
    }
    ((pone_int*)i)->i++;
    return i;
}

PONE_FUNC(meth_int_is_prime) {
    PONE_ARG("Int#is_prime", "");
    pone_int_t num = pone_intify(world, self);

    if (num <= 1) return pone_false();
    if (num == 2) return pone_true();
    if (num % 2 == 0) return pone_false();
    for(pone_int_t i = 3; i < num / 2; i+= 2) {
        if (num % i == 0) {
            return pone_false();
        }
    }
    return pone_true();
}

PONE_FUNC(meth_int_str) {
    PONE_ARG("Int#Str", "");
    return pone_str_from_int(world, pone_int_val(self));
}

PONE_FUNC(meth_int_int) {
    PONE_ARG("Int#Int", "");
    return pone_int_new(world, pone_int_val(self));
}

PONE_FUNC(meth_int_accepts) {
    pone_val* rhs;
    PONE_ARG("Int#accepts", "o", &rhs);

    bool b = pone_eq(world, self, rhs);
    return b ? pone_true() : pone_false();
}

PONE_FUNC(meth_int_num) {
    PONE_ARG("Int#Num", "");
    return pone_num_new(world, pone_int_val(self));
}

void pone_int_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_int == NULL);

    universe->class_int = pone_class_new(world, "Int", strlen("Int"));
    pone_class_push_parent(world, universe->class_int, universe->class_cool);
    PONE_REG_METHOD(universe->class_int, "is-prime", meth_int_is_prime);
    PONE_REG_METHOD(universe->class_int, "Str", meth_int_str);
    PONE_REG_METHOD(universe->class_int, "Int", meth_int_int);
    PONE_REG_METHOD(universe->class_int, "Num", meth_int_num);
    PONE_REG_METHOD(universe->class_int, "ACCEPTS", meth_int_accepts);
    pone_class_compose(world, universe->class_int);
}

