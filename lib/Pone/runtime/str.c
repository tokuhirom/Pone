#include "pone.h" /* PONE_INC */

// pone dup p.
pone_val* pone_new_str(pone_world* world, const char*p, size_t len) {
    pone_string* pv = (pone_string*)pone_obj_alloc(world, PONE_STRING);
    pv->p = pone_strdup(world, p, len);
    pv->len = len;
    return (pone_val*)pv;
}

// pone doesn't dup p.
pone_val* pone_new_str_const(pone_world* world, const char*p, size_t len) {
    pone_string* pv = (pone_string*)pone_obj_alloc(world, PONE_STRING);
    pv->flags |= PONE_FLAGS_STR_CONST | PONE_FLAGS_STR_FROZEN;
    pv->p = p;
    pv->len = len;
    return (pone_val*)pv;
}

void pone_str_free(pone_world* world, pone_val* val) {
    if (!(pone_flags(val) & PONE_FLAGS_STR_CONST)) {
        pone_free(world, (char*)((pone_string*)val)->p);
    }
}

pone_val* pone_str_from_int(pone_world* world, int i) {
    // INT_MAX=2147483647. "2147483647".elems = 10
    char buf[11+1];
    int size = snprintf(buf, 11+1, "%d", i);
    return pone_mortalize(world, pone_new_str(world, buf, size));
}

pone_val* pone_str_from_num(pone_world* world, double n) {
    char buf[512+1];
    int size = snprintf(buf, 512+1, "%f", n);
    return pone_mortalize(world, pone_new_str(world, buf, size));
}

// TODO: pone_new_str_const

/**
 * @return (mortalized)
 */
// TODO deprecate
pone_val* pone_str(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_NIL:
        return pone_mortalize(world, pone_new_str_const(world, "(undef)", strlen("(undef)")));
    case PONE_INT:
        return pone_str_from_int(world, pone_int_val(val));
    case PONE_NUM:
        return pone_str_from_num(world, pone_num_val(val));
    case PONE_STRING:
        return val;
    case PONE_BOOL:
        if (pone_bool_val(val)) {
            return pone_mortalize(world, pone_new_str_const(world, "True", strlen("True")));
        } else {
            return pone_mortalize(world, pone_new_str_const(world, "False", strlen("False")));
        }
    default:
        abort();
    }
}

/**
 * @return not mortalized
 */
pone_val* pone_to_str(pone_universe* universe, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_NIL:
        return pone_new_str_const(NULL, "(undef)", strlen("(undef)"));
    case PONE_STRING:
        return val;
    default:
        abort();
    }
}

inline const char* pone_string_ptr(pone_val* val) {
    assert(pone_type(val) == PONE_STRING);
    return ((pone_string*)val)->p;
}

inline size_t pone_string_len(pone_val* val) {
    assert(pone_type(val) == PONE_STRING);
    return ((pone_string*)val)->len;
}

