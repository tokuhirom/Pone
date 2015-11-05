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

pone_val* pone_str_new_vprintf(pone_universe* universe, const char* fmt, va_list args) {
    assert(universe);

    va_list copied;
    va_copy(copied, args);

    int size = vsnprintf(NULL, 0, fmt, args);
    if (size < 0) {
        abort();
    }
    char* p = pone_malloc(universe, size+1);
    size = vsnprintf(p, size+1, fmt, copied);
    if (size < 0) {
        abort();
    }

    pone_string* pv = (pone_string*)pone_obj_alloc(universe, PONE_STRING);
    pv->p = p;
    pv->len = size;
    return (pone_val*)pv;
}

pone_val* pone_str_concat(pone_world* world, pone_val* v1, pone_val* v2) {
    pone_val* s1 = pone_stringify(world, v1);
    pone_val* s2 = pone_stringify(world, v2);

    pone_string* pv = (pone_string*)pone_obj_alloc(world->universe, PONE_STRING);
    pv->len = pone_str_len(s1)+pone_str_len(s2);
    pv->p = pone_malloc(world->universe, pv->len);
    memcpy(pv->p, pone_str_ptr(s1), pone_str_len(s1));
    memcpy(pv->p+pone_str_len(s1), pone_str_ptr(s2), pone_str_len(s2));

    pone_refcnt_dec(world->universe, s1);
    pone_refcnt_dec(world->universe, s2);

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
    return pone_call_method(world, val, "Str", 0);
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
        pone_throw_str(world, "You can't modify immutable string");
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

static pone_val* meth_str_str(pone_world* world, pone_val* self, int n, va_list args) {
    return pone_str_copy(world->universe, self);
}

void pone_str_init(pone_universe* universe) {
    assert(universe->class_str == NULL);

    universe->class_str = pone_class_new(universe, "Str", strlen("Str"));
    pone_class_push_parent(universe, universe->class_str, universe->class_cool);
    pone_add_method_c(universe, universe->class_str, "Str", strlen("Str"), meth_str_str);
}

