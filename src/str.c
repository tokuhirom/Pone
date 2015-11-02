#include "pone.h" /* PONE_INC */

// pone dup p.
pone_val* pone_str_new(pone_universe* universe, const char*p, size_t len) {
    pone_string* pv = (pone_string*)pone_obj_alloc(universe, PONE_STRING);
    pv->p = pone_strdup(universe, p, len);
    pv->len = len;
    return (pone_val*)pv;
}

// pone doesn't dup p.
pone_val* pone_str_new_const(pone_universe* universe, const char*p, size_t len) {
    assert(universe);
    pone_string* pv = (pone_string*)pone_obj_alloc(universe, PONE_STRING);
    pv->flags |= PONE_FLAGS_STR_CONST | PONE_FLAGS_FROZEN;
    pv->p = (char*)p;
    pv->len = len;
    return (pone_val*)pv;
}

pone_val* pone_str_copy(pone_universe* universe, pone_val* val) {
    assert(pone_type(val) == PONE_STRING);
    pone_string* pv = (pone_string*)pone_obj_alloc(universe, PONE_STRING);
    pv->flags |= PONE_FLAGS_STR_COPY | PONE_FLAGS_FROZEN;
    pv->val = val;
    pone_refcnt_inc(universe, val);
    pv->len = 0;
    return (pone_val*)pv;
}

void pone_str_free(pone_universe* universe, pone_val* val) {
    if (pone_flags(val) & PONE_FLAGS_STR_COPY) {
        pone_refcnt_dec(universe, val->as.str.val);
    } else if (!(pone_flags(val) & PONE_FLAGS_STR_CONST)) {
        pone_free(universe, (char*)((pone_string*)val)->p);
    }
}

pone_val* pone_str_from_int(pone_universe* universe, int i) {
    // INT_MAX=2147483647. "2147483647".elems = 10
    char buf[11+1];
    int size = snprintf(buf, 11+1, "%d", i);
    return pone_str_new(universe, buf, size);
}

pone_val* pone_str_from_num(pone_universe* universe, double n) {
    char buf[512+1];
    int size = snprintf(buf, 512+1, "%f", n);
    return pone_str_new(universe, buf, size);
}

/**
 * @return not mortalized
 */
pone_val* pone_stringify(pone_world* world, pone_val* val) {
    pone_universe* universe = world->universe;
    switch (pone_type(val)) {
    case PONE_NIL:
        return pone_str_new_const(universe, "(undef)", strlen("(undef)"));
    case PONE_INT:
        return pone_str_from_int(universe, pone_int_val(val));
    case PONE_STRING:
        return pone_str_copy(universe, val);
    case PONE_NUM:
        return pone_str_from_num(universe, pone_num_val(val));
    case PONE_BOOL:
        if (pone_bool_val(val)) {
            return pone_str_new_const(universe, "True", strlen("True"));
        } else {
            return pone_str_new_const(universe, "False", strlen("False"));
        }
    case PONE_OBJ:
        return pone_call_method(world, val, "Str", 0);
    default:
        abort(); // TODO
    }
}

inline const char* pone_str_ptr(pone_val* val) {
    assert(pone_type(val) == PONE_STRING);
    if (pone_flags(val) & PONE_FLAGS_STR_COPY) {
        return pone_str_ptr(val->as.str.val);
    } else {
        return ((pone_string*)val)->p;
    }
}

inline size_t pone_str_len(pone_val* val) {
    assert(pone_type(val) == PONE_STRING);
    if (pone_flags(val) & PONE_FLAGS_STR_COPY) {
        return pone_str_len(val->as.str.val);
    } else {
        return ((pone_string*)val)->len;
    }
}

void pone_str_append_c(pone_world* world, pone_val* val, const char* s, int s_len) {
    pone_universe* universe = world->universe;

    if (pone_flags(val) & PONE_FLAGS_STR_COPY) { // needs CoW
        pone_val* src = val->as.str.val;
        val->as.str.p = pone_strdup(universe, pone_str_ptr(src), pone_str_len(src));
        val->as.str.len = pone_str_len(src);
        val->as.basic.flags ^= PONE_FLAGS_STR_COPY;
        // refcnt-- for source val
        pone_refcnt_dec(universe, val->as.str.val);
    } else if (pone_flags(val) & PONE_FLAGS_STR_CONST) {
        pone_die_str(world, "You can't modify immutable string");
    }

    val->as.str.p = realloc(val->as.str.p, val->as.str.len + s_len);
    PONE_ALLOC_CHECK(val->as.str.p);
    memcpy(val->as.str.p + val->as.str.len, s, s_len);
    val->as.str.len += s_len;
}

void pone_str_append(pone_world* world, pone_val* str, pone_val* s) {
    s = pone_stringify(world, s);
    pone_str_append_c(world, str, pone_str_ptr(s), pone_str_len(s));
    pone_refcnt_dec(world->universe, s);
}

