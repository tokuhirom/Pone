#include "pone.h" /* PONE_INC */

void pone_str_mark(pone_val* val) {
    if (pone_type(val) == PONE_FLAGS_STR_COPY) {
        pone_gc_mark_value(val->as.str.val);
    }
}


// pone dup p.
pone_val* pone_str_new(pone_world* world, const char*p, size_t len) {
    pone_string* pv = (pone_string*)pone_obj_alloc(world, PONE_STRING);
    pv->p = pone_strdup(world, p, len);
    pv->len = len;
    return (pone_val*)pv;
}

// pone doesn't dup p.
pone_val* pone_str_new_const(pone_world* world, const char*p, size_t len) {
    assert(world);
    pone_string* pv = (pone_string*)pone_obj_alloc(world, PONE_STRING);
    pv->flags |= PONE_FLAGS_STR_CONST | PONE_FLAGS_FROZEN;
    pv->p = (char*)p;
    pv->len = len;
    return (pone_val*)pv;
}

pone_val* pone_str_new_printf(pone_world* world, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    pone_val* retval = pone_str_new_vprintf(world, fmt, args);
    va_end(args);
    return retval;
}

pone_val* pone_str_new_vprintf(pone_world* world, const char* fmt, va_list args) {
    assert(world);

    va_list copied;
    va_copy(copied, args);

    int size = vsnprintf(NULL, 0, fmt, args);
    if (size < 0) {
        abort();
    }
    char* p = pone_malloc(world->universe, size+1);
    size = vsnprintf(p, size+1, fmt, copied);
    if (size < 0) {
        abort();
    }

    pone_string* pv = (pone_string*)pone_obj_alloc(world, PONE_STRING);
    pv->p = p;
    pv->len = size;
    return (pone_val*)pv;
}

pone_val* pone_str_concat(pone_world* world, pone_val* v1, pone_val* v2) {
    pone_val* s1 = pone_stringify(world, v1);
    pone_val* s2 = pone_stringify(world, v2);

    pone_string* pv = (pone_string*)pone_obj_alloc(world, PONE_STRING);
    pv->len = pone_str_len(s1)+pone_str_len(s2);
    pv->p = pone_malloc(world->universe, pv->len);
    memcpy(pv->p, pone_str_ptr(s1), pone_str_len(s1));
    memcpy(pv->p+pone_str_len(s1), pone_str_ptr(s2), pone_str_len(s2));

    return (pone_val*)pv;
}


pone_val* pone_str_copy(pone_world* world, pone_val* val) {
    assert(pone_type(val) == PONE_STRING);
    pone_string* pv = (pone_string*)pone_obj_alloc(world, PONE_STRING);
    pv->flags |= PONE_FLAGS_STR_COPY | PONE_FLAGS_FROZEN;
    pv->val = val;
    pv->len = 0;
    return (pone_val*)pv;
}

void pone_str_free(pone_universe* universe, pone_val* val) {
    if (pone_flags(val) & PONE_FLAGS_STR_COPY) {
        // there's no heap ref.
    } else if (!(pone_flags(val) & PONE_FLAGS_STR_CONST)) {
        pone_free(universe, (char*)((pone_string*)val)->p);
    }
}

pone_val* pone_str_from_int(pone_world* world, pone_int_t i) {
    // LONG_MAX=9223372036854775807. "9223372036854775807".elems = 19
    char buf[19+1];
    int size = snprintf(buf, 19+1, PoneIntFmt, i);
    return pone_str_new(world, buf, size);
}

pone_val* pone_str_from_num(pone_world* world, double n) {
    char buf[512+1];
    int size = snprintf(buf, 512+1, "%f", n);
    return pone_str_new(world, buf, size);
}

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

bool pone_str_contains_null(pone_universe* universe, pone_val* val) {
    assert(pone_type(val) == PONE_STRING);
    return memchr(pone_str_ptr(val), 0, pone_str_len(val)) != NULL;
}

pone_val* pone_str_c_str(pone_world* world, pone_val* val) {
    assert(pone_type(val) == PONE_STRING);
    if (pone_str_contains_null(world->universe, val)) {
        pone_throw_str(world, "You can't convert string to c-string. Since it contains \\0.");
    }

    return pone_str_concat(world, val, pone_str_new(world, "\0", 1));
}

void pone_str_append_c(pone_world* world, pone_val* val, const char* s, pone_int_t s_len) {
    if (pone_flags(val) & PONE_FLAGS_STR_COPY) { // needs CoW
        pone_val* src = val->as.str.val;
        char* p = pone_strdup(world, pone_str_ptr(src), pone_str_len(src));
        val->as.str.len = pone_str_len(src);
        val->as.basic.flags ^= PONE_FLAGS_STR_COPY;
        val->as.str.p = p;
    } else if (pone_flags(val) & PONE_FLAGS_STR_CONST) {
        pone_throw_str(world, "You can't modify immutable string");
    }

    assert(pone_type(val) == PONE_STRING);
    val->as.str.p = realloc(val->as.str.p, val->as.str.len + s_len);
    PONE_ALLOC_CHECK(val->as.str.p);
    memcpy(val->as.str.p + val->as.str.len, s, s_len);
    val->as.str.len += s_len;
}

void pone_str_append(pone_world* world, pone_val* str, pone_val* s) {
    s = pone_stringify(world, s);
    pone_str_append_c(world, str, pone_str_ptr(s), pone_str_len(s));
}

void pone_str_appendf(pone_world* world, pone_val* str, const char* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    pone_str_append(world, str, pone_str_new_vprintf(world, fmt, args));
    va_end(args);
}

static pone_val* meth_str_str(pone_world* world, pone_val* self, int n, va_list args) {
    return pone_str_copy(world, self);
}

static pone_val* meth_str_int(pone_world* world, pone_val* self, int n, va_list args) {
    char *end = (char*)pone_str_ptr(self) + pone_str_len(self);
    return pone_int_new(world, strtol(pone_str_ptr(self), &end, 10));
}

static pone_val* meth_str_num(pone_world* world, pone_val* self, int n, va_list args) {
    char *end = (char*)pone_str_ptr(self) + pone_str_len(self);
    return pone_num_new(world, strtod(pone_str_ptr(self), &end));
}

void pone_str_init(pone_world* world) {
    pone_universe * universe = world->universe;
    assert(universe->class_str == NULL);

    universe->class_str = pone_class_new(world, "Str", strlen("Str"));
    pone_class_push_parent(world, universe->class_str, universe->class_cool);
    pone_add_method_c(world, universe->class_str, "Str", strlen("Str"), meth_str_str);
    pone_add_method_c(world, universe->class_str, "Int", strlen("Int"), meth_str_int);
    pone_add_method_c(world, universe->class_str, "Num", strlen("Num"), meth_str_num);
    pone_class_compose(world, universe->class_str);
}


