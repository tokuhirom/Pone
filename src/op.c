#include "pone.h" /* PONE_INC */
#include <setjmp.h>

pone_val* pone_get_lex(pone_world* world, const char* key) {
    pone_lex_t* lex = world->lex;
    while (lex != NULL) {
        khint_t kh = kh_get(str, lex->map, key);
        if (kh == kh_end(lex->map)) {
            lex = lex->parent;
            continue;
        }
        return kh_val(lex->map, kh);
    }
    fprintf(stderr, "unknown lexical variable: %s\n", key);
    abort();
}

pone_val* pone_assign(pone_world* world, int up, const char* key, pone_val* val) {
    pone_lex_t* lex = world->lex;
    for (int i=0; i<up; i++) {
        lex = lex->parent;
    }

    pone_refcnt_inc(world->universe, val);
    int ret;
    khint_t k = kh_put(str, lex->map, key, &ret);
    if (ret == -1) {
        fprintf(stderr, "hash operation failed\n");
        abort();
    }
    if (ret == 0) { // the key is present in the hash table
        pone_refcnt_dec(world->universe, kh_val(lex->map, k));
    }
    kh_val(lex->map, k) = val;

    return val;
}

// TODO we should implement .gist and .perl method for each class...
void pone_dd(pone_universe* universe, pone_val* val) {
    switch (pone_type(val)) {
        case PONE_STRING:
            printf("(string: len:%d, ", pone_str_len(val));
            fwrite(pone_str_ptr(val), 1, pone_str_len(val), stdout);
            printf(")\n");
            break;
        case PONE_INT:
            printf("(int: refcnt:%d, flags:%d %d)\n", pone_refcnt(val), pone_flags(val), pone_int_val(val));
            break;
        case PONE_NIL:
            printf("(undef)\n");
            break;
        case PONE_CODE:
            printf("(code)\n");
            break;
        case PONE_HASH: {
            printf("(hash ");
            const char* k;
            pone_val* v;
            kh_foreach(val->as.hash.h, k, v, {
                printf("key:%s, ", k);
            });
            printf(")\n");
            break;
        }
        case PONE_ARRAY: {
            printf("(array len:%d, max:%d)\n", val->as.ary.len, val->as.ary.max);
            break;
        }
        case PONE_OBJ: {
            printf("(obj)\n");
            break;
        }
        default:
            fprintf(stderr, "unknown type: %d\n", pone_type(val));
            abort();
    }
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

int pone_intify(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_NIL:
        pone_throw_str(world, "Use of uninitialized value as integer");
        abort();
    case PONE_INT:
        return pone_int_val(val);
    case PONE_STRING: {
        char *end = (char*)pone_str_ptr(val) + pone_str_len(val);
        return strtol(pone_str_ptr(val), &end, 10);
    }
    case PONE_CODE:
        pone_throw_str(world, "you can't convert CODE to integer");
    default:
        pone_throw_str(world, "you can't convert this type to integer");
        abort();
    }
}

pone_num_t pone_numify(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_NIL:
        pone_throw_str(world, "Use of uninitialized value as num");
        abort();
    case PONE_INT:
        return pone_int_val(val);
    case PONE_NUM:
        return val->as.num.n;
    case PONE_STRING: {
        char *end = (char*)pone_str_ptr(val) + pone_str_len(val);
        return strtod(pone_str_ptr(val), &end);
    }
    case PONE_CODE:
        pone_throw_str(world, "you can't convert CODE to num");
        break;
    default:
        pone_throw_str(world, "you can't convert this type to num");
        abort();
    }
}

pone_val* pone_add(pone_world* world, pone_val* v1, pone_val* v2) {
    if (pone_type(v1) == PONE_NUM || pone_type(v2) == PONE_NUM) {
        pone_num_t n1 = pone_numify(world, v1);
        pone_num_t n2 = pone_numify(world, v2);
        return pone_num_new(world->universe, n1 + n2);
    } else {
        int i1 = pone_intify(world, v1);
        int i2 = pone_intify(world, v2);
        return pone_int_new(world->universe, i1 + i2);
    }
}

pone_val* pone_subtract(pone_world* world, pone_val* v1, pone_val* v2) {
    if (pone_type(v1) == PONE_NUM || pone_type(v2) == PONE_NUM) {
        pone_num_t n1 = pone_numify(world, v1);
        pone_num_t n2 = pone_numify(world, v2);
        return pone_num_new(world->universe, n1 - n2);
    } else {
        int i1 = pone_intify(world, v1);
        int i2 = pone_intify(world, v2);
        return pone_int_new(world->universe, i1 - i2);
    }
}

pone_val* pone_multiply(pone_world* world, pone_val* v1, pone_val* v2) {
    if (pone_type(v1) == PONE_NUM || pone_type(v2) == PONE_NUM) {
        pone_num_t n1 = pone_numify(world, v1);
        pone_num_t n2 = pone_numify(world, v2);
        return pone_num_new(world->universe, n1 * n2);
    } else {
        int i1 = pone_intify(world, v1);
        int i2 = pone_intify(world, v2);
        return pone_int_new(world->universe, i1 * i2);
    }
}

pone_val* pone_divide(pone_world* world, pone_val* v1, pone_val* v2) {
    if (pone_type(v1) == PONE_NUM || pone_type(v2) == PONE_NUM) {
        pone_num_t n1 = pone_numify(world, v1);
        pone_num_t n2 = pone_numify(world, v2);
        return pone_num_new(world->universe, n1 / n2);
    } else {
        int i1 = pone_intify(world, v1);
        int i2 = pone_intify(world, v2);
        return pone_int_new(world->universe, i1 / i2);
    }
}

pone_val* pone_mod(pone_world* world, pone_val* v1, pone_val* v2) {
    int i1 = pone_intify(world, v1);
    int i2 = pone_intify(world, v2);
    return pone_int_new(world->universe, i1 % i2); // TODO: We should upgrade value to NV
}

#define CMP_OP(op) \
    do { \
        if (pone_type(v1) == PONE_NUM || pone_type(v2) == PONE_NUM) { \
            pone_num_t n1 = pone_numify(world, v1); \
            pone_num_t n2 = pone_numify(world, v2); \
            return n1 op n2; \
        } else { \
            int i1 = pone_intify(world, v1); \
            int i2 = pone_intify(world, v2); \
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

int pone_str_cmp(pone_world* world, pone_val* v1, pone_val* v2) {
    pone_val* s1 = pone_stringify(world, v1);
    pone_val* s2 = pone_stringify(world, v2);
    int l1 = pone_str_len(s1);
    int l2 = pone_str_len(s2);
    int n = memcmp(pone_str_ptr(s1), pone_str_ptr(s2), MIN(pone_str_len(s1), pone_str_len(s2)));
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
    }
}

const char* pone_what_str_c(pone_val* val) {
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
    case PONE_OBJ: {
        return "Obj"; // TODO return the class name!
    }
    }
    if (pone_type(val) == 0) {
        fprintf(stderr, "[ERROR] You can't access free'd value: %x\n",
                val);
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

