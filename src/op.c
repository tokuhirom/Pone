#include "pone.h" /* PONE_INC */
#include <setjmp.h>

pone_val* pone_get_lex(pone_world* world, const char* key) {
    pone_val* lex = world->lex;
    while (lex != NULL) {
        assert(pone_alive(lex));
        khint_t kh = kh_get(str, lex->as.lex.map, key);
        if (kh == kh_end(lex->as.lex.map)) {
            lex = lex->as.lex.parent;
            continue;
        }
        return kh_val(lex->as.lex.map, kh);
    }

    {
        khint_t k = kh_get(str, world->universe->globals, key);
        if (k != kh_end(world->universe->globals)) {
            return kh_val(world->universe->globals, k);
        }
    }

    pone_throw_str(world, "Unknown lexical variable: %s", key);
    abort();
}

pone_val* pone_assign(pone_world* world, int up, const char* key, pone_val* val) {
    pone_val* lex = world->lex;
#ifndef NDEBUG
    if (pone_type(world->lex) != PONE_LEX) {
        printf("illegal value %p. type:%d\n", val, pone_type(val));
        abort();
    }
#endif

    assert(pone_type(world->lex) == PONE_LEX);
    for (int i=0; i<up; i++) {
        lex = lex->as.lex.parent;
    }

    if (pthread_self() != lex->as.lex.thread_id) {
        pone_throw_str(world, "You can't set variable to non-owned lexical variables from other thread");
    }

    GC_LOCK(world->universe);
    int ret;
    khint_t k = kh_put(str, lex->as.lex.map, key, &ret);
    if (ret == -1) {
        fprintf(stderr, "hash operation failed\n");
        abort();
    }
    kh_val(lex->as.lex.map, k) = val;
    GC_UNLOCK(world->universe);

    return val;
}

// This function is used by following Perl6 code:
//
//     $var[$pos] = $rhs
//
// It's same as following code:
//
//     $var.ASSIGN-POS($pos, $rhs);
pone_val* pone_assign_pos(pone_world* world, pone_val* var, pone_val* pos, pone_val* rhs) {
    if (pone_type(var) == PONE_ARRAY) { // specialize
        pone_ary_assign_pos(world, var, pos, rhs);
        return rhs;
    } else {
        return pone_call_method(world, var, "ASSIGN-POS", 2, pos, rhs);
    }
}

// This function is used by following Perl6 code:
//
//     $h<$key> = $rhs
//
// It's same as following code:
//
//     $var.ASSIGN-KEY($key, $rhs);
pone_val* pone_assign_key(pone_world* world, pone_val* var, pone_val* key, pone_val* rhs) {
    return pone_call_method(world, var, "ASSIGN-KEY", 2, key, rhs);
}

static void pin(pone_int_t indent) {
    for (pone_int_t i=0; i<indent; ++i) {
        printf(" ");
    }
}

static void dd(pone_universe* universe, pone_val* val, pone_int_t indent) {
    pin(indent);
    switch (pone_type(val)) {
        case PONE_STRING:
            printf("(string: len:" PoneIntFmt " , ", pone_str_len(val));
            fwrite(pone_str_ptr(val), 1, pone_str_len(val), stdout);
            printf(")\n");
            break;
        case PONE_INT:
            printf("(int: flags:%d " PoneIntFmt ")\n", pone_flags(val), pone_int_val(val));
            break;
        case PONE_NIL:
            printf("(undef)\n");
            break;
        case PONE_CODE:
            printf("(code)\n");
            break;
        case PONE_HASH: {
            printf("(hash\n");
            const char* k;
            pone_val* v;
            kh_foreach(val->as.hash.h, k, v, {
                pin(indent+1);
                printf("key:%s\n", k);
                dd(universe, v, indent+2);
            });
            pin(indent);
            printf(")\n");
            break;
        }
        case PONE_ARRAY: {
            printf("(array len:" PoneIntFmt ", max:" PoneIntFmt, val->as.ary.len, val->as.ary.max);
            for (pone_int_t i=0; i<val->as.ary.len; ++i) {
                pin(indent+1);
                dd(universe, val->as.ary.a[i], indent+2);
            }
            pin(indent);
            printf(")\n");
            break;
        }
        case PONE_BOOL: {
            printf("(bool %s)\n", val->as.boolean.b ? "true" : "false");
            break;
        }
        case PONE_OBJ: {
            printf("(obj\n");
            pin(indent+1);
            if (val->as.obj.klass) {
                printf("class:\n");
                dd(universe, val->as.obj.klass, indent+2);
            } else {
                printf("class is NULL!\n");
            }
            const char* k;
            pone_val* v;
            kh_foreach(val->as.obj.ivar, k, v, {
                pin(indent+1);
                printf("key:%s\n", k);
                dd(universe, v, indent+2);
            });
            pin(indent);
            printf(")\n");
            break;
        }
        default:
            fprintf(stderr, "unknown type: %d\n", pone_type(val));
            abort();
    }
}

// TODO we should implement .gist and .perl method for each class...
void pone_dd(pone_world* world, pone_val* val) {
    dd(world->universe, val, 0);
}


bool pone_so(pone_val* val) {
    switch (pone_type(val)) {
    case PONE_INT:
        return pone_int_val(val) != 0;
    case PONE_NUM:
        return pone_int_val(val) != 0;
    case PONE_STRING:
        return pone_str_len(val) != 0;
    case PONE_BOOL:
        return pone_bool_val(val);
    default:
        return true;
    }
}

pone_int_t pone_intify(pone_world* world, pone_val* val) {
    pone_val* v = pone_call_method(world, val, "Int", 0);
    return pone_int_val(v);
}

pone_num_t pone_numify(pone_world* world, pone_val* val) {
    pone_val* v = pone_call_method(world, val, "Num", 0);
    return pone_num_val(v);
}

pone_val* pone_smart_match(pone_world* world, pone_val* v1, pone_val* v2) {
    return pone_call_method(world, v2, "ACCEPTS", 1, v1);
}

pone_val* pone_add(pone_world* world, pone_val* v1, pone_val* v2) {
    if (pone_type(v1) == PONE_NUM || pone_type(v2) == PONE_NUM) {
        pone_num_t n1 = pone_numify(world, v1);
        pone_num_t n2 = pone_numify(world, v2);
        return pone_num_new(world, n1 + n2);
    } else {
        pone_int_t i1 = pone_intify(world, v1);
        pone_int_t i2 = pone_intify(world, v2);
        return pone_int_new(world, i1 + i2);
    }
}

pone_val* pone_subtract(pone_world* world, pone_val* v1, pone_val* v2) {
    if (pone_type(v1) == PONE_NUM || pone_type(v2) == PONE_NUM) {
        pone_num_t n1 = pone_numify(world, v1);
        pone_num_t n2 = pone_numify(world, v2);
        return pone_num_new(world, n1 - n2);
    } else {
        pone_int_t i1 = pone_intify(world, v1);
        pone_int_t i2 = pone_intify(world, v2);
        return pone_int_new(world, i1 - i2);
    }
}

pone_val* pone_multiply(pone_world* world, pone_val* v1, pone_val* v2) {
    if (pone_type(v1) == PONE_NUM || pone_type(v2) == PONE_NUM) {
        pone_num_t n1 = pone_numify(world, v1);
        pone_num_t n2 = pone_numify(world, v2);
        return pone_num_new(world, n1 * n2);
    } else {
        pone_int_t i1 = pone_intify(world, v1);
        pone_int_t i2 = pone_intify(world, v2);
        return pone_int_new(world, i1 * i2);
    }
}

pone_val* pone_divide(pone_world* world, pone_val* v1, pone_val* v2) {
    if (pone_type(v1) == PONE_NUM || pone_type(v2) == PONE_NUM) {
        pone_num_t n1 = pone_numify(world, v1);
        pone_num_t n2 = pone_numify(world, v2);
        return pone_num_new(world, n1 / n2);
    } else {
        pone_int_t i1 = pone_intify(world, v1);
        pone_int_t i2 = pone_intify(world, v2);
        return pone_int_new(world, i1 / i2);
    }
}

pone_val* pone_mod(pone_world* world, pone_val* v1, pone_val* v2) {
    pone_int_t i1 = pone_intify(world, v1);
    pone_int_t i2 = pone_intify(world, v2);
    return pone_int_new(world, i1 % i2); // TODO: We should upgrade value to NV
}

#define CMP_OP(op) \
    do { \
        if (pone_type(v1) == PONE_NUM || pone_type(v2) == PONE_NUM) { \
            pone_num_t n1 = pone_numify(world, v1); \
            pone_num_t n2 = pone_numify(world, v2); \
            return n1 op n2; \
        } else { \
            pone_int_t i1 = pone_intify(world, v1); \
            pone_int_t i2 = pone_intify(world, v2); \
            return i1 op i2; \
        } \
    } while (0)

bool pone_eq(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(==); }
bool pone_ne(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(!=); }
bool pone_le(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(<=); }
bool pone_lt(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(<);  }
bool pone_ge(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(>=); }
bool pone_gt(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(>);  }

#undef CMP_OP

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

pone_int_t pone_str_cmp(pone_world* world, pone_val* v1, pone_val* v2) {
    pone_val* s1 = pone_stringify(world, v1);
    pone_val* s2 = pone_stringify(world, v2);
    pone_int_t l1 = pone_str_len(s1);
    pone_int_t l2 = pone_str_len(s2);
    pone_int_t n = memcmp(pone_str_ptr(s1), pone_str_ptr(s2), MIN(pone_str_len(s1), pone_str_len(s2)));
    if (n == 0) {
        if (l1 > l2) {
            n = 1;
        } else if (l1 < l2) {
            n = -1;
        }
    }
    return n;
}

#define CMP_OP(op) return pone_str_cmp(world, v1, v2) op

bool pone_str_eq(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(== 0); }
bool pone_str_ne(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(!= 0); }
bool pone_str_le(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(<= 0); }
bool pone_str_lt(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(<  0); }
bool pone_str_ge(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(>= 0); }
bool pone_str_gt(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(>  0); }

#undef CMP_OP

size_t pone_elems(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_STRING:
        return pone_str_len(val);
    case PONE_ARRAY:
        return pone_ary_elems(val);
    case PONE_HASH:
        return pone_hash_elems(val);
    case PONE_NIL:
    case PONE_INT:
    case PONE_NUM:
    case PONE_BOOL:
    case PONE_CODE:
        return 1; // same as perl6
    case PONE_OBJ:
        abort(); // TODO call .elem?
    case PONE_LEX:
        abort();
    }
    abort();
}

const char* pone_what_str_c(pone_val* val) {
    assert(pone_alive(val));
    switch (pone_type(val)) {
    case PONE_NIL:
        return "Any"; // remove this branch. pone_call_method(pone_what(), "name") should work.
    case PONE_INT:
        return "Int";
    case PONE_NUM:
        return "Num";
    case PONE_STRING:
        return "Str";
    case PONE_ARRAY:
        return "Array";
    case PONE_BOOL:
        return "Bool";
    case PONE_HASH:
        return "Hash";
    case PONE_CODE:
        return "Code";
    case PONE_OBJ:
        return "Obj"; // TODO return the class name!
    case PONE_LEX:
        abort();
    }
    abort();
}

bool pone_is_frozen(pone_val* v) {
    return pone_flags(v) & PONE_FLAGS_FROZEN;
}

// Call AT-POS
pone_val* pone_at_pos(pone_world* world, pone_val* obj, pone_val* pos) {
    if (pone_type(obj) == PONE_ARRAY) { // specialization for performance
        return pone_ary_at_pos(obj, pone_intify(world, pos));
    } else {
        return pone_call_method(world, obj, "AT-POS", 1, pos);
    }
}

pone_val* pone_at_key(pone_world* world, pone_val* obj, pone_val* pos) {
    if (pone_type(obj) == PONE_HASH) { // specialization for performance
        return pone_hash_at_key_c(world->universe, obj, pone_str_ptr(pone_str_c_str(world, pos)));
    } else {
        return pone_call_method(world, obj, "AT-KEY", 1, pos);
    }
}

