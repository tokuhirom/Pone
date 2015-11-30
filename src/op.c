#include "pone.h" /* PONE_INC */
#include "pone_exc.h"
#include <math.h>
#include <setjmp.h>
#include <ctype.h>

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
    for (int i = 0; i < up; i++) {
        lex = lex->as.lex.parent;
    }

    if (pthread_self() != lex->as.lex.thread_id) {
        pone_throw_str(world, "You can't set variable to non-owned lexical variables from other thread");
    }

    int ret;
    khint_t k = kh_put(str, lex->as.lex.map, key, &ret);
    if (ret == -1) {
        fprintf(stderr, "hash operation failed\n");
        abort();
    }
    kh_val(lex->as.lex.map, k) = val;

    return val;
}

#define INFIX(funcname, op_func)                                                           \
    pone_val* funcname(pone_world* world, int level, const char* varname, pone_val* val) { \
        pone_val* lhs = pone_get_lex(world, varname);                                      \
        pone_val* result = op_func(world, lhs, val);                                       \
        return pone_assign(world, level, varname, result);                                 \
    }

INFIX(pone_inplace_add, pone_add)
INFIX(pone_inplace_sub, pone_subtract)
INFIX(pone_inplace_mul, pone_multiply)
INFIX(pone_inplace_mod, pone_mod)
INFIX(pone_inplace_div, pone_divide)
INFIX(pone_inplace_pow, pone_pow)
INFIX(pone_inplace_bin_or, pone_bitwise_or)
INFIX(pone_inplace_bin_and, pone_bitwise_and)
INFIX(pone_inplace_bin_xor, pone_bitwise_xor)
INFIX(pone_inplace_blshift, pone_blshift)
INFIX(pone_inplace_brshift, pone_brshift)
INFIX(pone_inplace_concat_s, pone_str_concat)

#undef INFIX

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
    for (pone_int_t i = 0; i < indent; ++i) {
        printf(" ");
    }
}

static void dd(pone_world* world, pone_val* val, pone_int_t indent) {
    pin(indent);
    switch (pone_type(val)) {
    case PONE_STRING: {
        printf("(string: immutable:%d len:" PoneIntFmt " , ",
               pone_flags(val) & PONE_FLAGS_FROZEN,
               pone_str_len(val));
        for (pone_int_t i = 0; i < pone_str_len(val); ++i) {
            if (isprint(*(pone_str_ptr(val) + i))) {
                fwrite(pone_str_ptr(val) + i, 1, 1, stdout);
            } else {
                char* p = pone_str_ptr(val);
                printf("\\x%02x", *(p + i));
            }
        }
        printf(")\n");
        break;
    }
    case PONE_INT:
        printf("(int: flags:%d " PoneIntFmt ")\n", pone_flags(val), pone_int_val(val));
        break;
    case PONE_NIL:
        printf("(undef)\n");
        break;
    case PONE_CODE:
        printf("(code func:%p)\n", val->as.code.func);
        break;
    case PONE_MAP: {
        printf("MAP:\n");
        pone_val* keys = pone_map_keys(world, val);
        for (pone_int_t i=0; i<pone_ary_size(keys); i++) {
            pone_val* key = pone_ary_at_pos(keys, i);
            pin(indent+1);
            printf("key:\n");
            dd(world, key, indent+2);
            pin(indent+1);
            printf("value:\n");
            pone_val* v = pone_map_at_key(world, val, key);
            dd(world, v, indent+2);
        }
        pin(indent);
        printf(")\n");
        break;
    }
    case PONE_ARRAY: {
        printf("(array len:" PoneIntFmt ", max:" PoneIntFmt "\n", val->as.ary.len, val->as.ary.max);
        for (pone_int_t i = 0; i < val->as.ary.len; ++i) {
            pin(indent + 1);
            dd(world, val->as.ary.a[i], indent + 2);
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
        pin(indent + 1);
        if (val->as.obj.klass) {
            printf("class:\n");
            dd(world, val->as.obj.klass, indent + 2);
        } else {
            printf("class is NULL!\n");
        }
        const char* k;
        pone_val* v;
        kh_foreach(val->as.obj.ivar, k, v, {
                pin(indent+1);
                printf("key:%s\n", k);
                dd(world, v, indent+2);
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
    dd(world, val, 0);
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

pone_val* pone_pow(pone_world* world, pone_val* v1, pone_val* v2) {
    pone_num_t n1 = pone_numify(world, v1);
    pone_num_t n2 = pone_numify(world, v2);
    pone_num_t n3 = pow(n1, n2);
    pone_world_set_errno(world);
    return pone_num_new(world, n3);
}

#define CMP_OP(op)                                                    \
    do {                                                              \
        if (pone_type(v1) == PONE_NUM || pone_type(v2) == PONE_NUM) { \
            pone_num_t n1 = pone_numify(world, v1);                   \
            pone_num_t n2 = pone_numify(world, v2);                   \
            return n1 op n2;                                          \
        } else {                                                      \
            pone_int_t i1 = pone_intify(world, v1);                   \
            pone_int_t i2 = pone_intify(world, v2);                   \
            return i1 op i2;                                          \
        }                                                             \
    } while (0)

bool pone_eq(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(== ); }
bool pone_ne(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(!= ); }
bool pone_le(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(<= ); }
bool pone_lt(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(< ); }
bool pone_ge(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(>= ); }
bool pone_gt(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(> ); }

#undef CMP_OP

#define BIT_OP(op)                              \
    do {                                        \
        pone_int_t i1 = pone_intify(world, v1); \
        pone_int_t i2 = pone_intify(world, v2); \
        return pone_int_new(world, i1 op i2);   \
    } while (0)

pone_val* pone_bitwise_or(pone_world* world, pone_val* v1, pone_val* v2) { BIT_OP(| ); }
pone_val* pone_bitwise_and(pone_world* world, pone_val* v1, pone_val* v2) { BIT_OP(&); }
pone_val* pone_bitwise_xor(pone_world* world, pone_val* v1, pone_val* v2) { BIT_OP (^); }
pone_val* pone_brshift(pone_world* world, pone_val* v1, pone_val* v2) { BIT_OP(>> ); }
pone_val* pone_blshift(pone_world* world, pone_val* v1, pone_val* v2) { BIT_OP(<< ); }

#undef BIT_OP

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
bool pone_str_lt(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(< 0); }
bool pone_str_ge(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(>= 0); }
bool pone_str_gt(pone_world* world, pone_val* v1, pone_val* v2) { CMP_OP(> 0); }

#undef CMP_OP

const char* pone_type_name(pone_val* val) {
    assert(pone_alive(val));
    switch (pone_type(val)) {
    case PONE_NIL:
        return "PONE_NIL";
    case PONE_INT:
        return "PONE_INT";
    case PONE_NUM:
        return "PONE_NUM";
    case PONE_STRING:
        return "PONE_STRING";
    case PONE_ARRAY:
        return "PONE_ARRAY";
    case PONE_BOOL:
        return "PONE_BOOL";
    case PONE_MAP:
        return "PONE_MAP";
    case PONE_CODE:
        return "PONE_CODE";
    case PONE_OBJ:
        return "PONE_OBJ";
    case PONE_LEX:
        return "PONE_LEX";
    case PONE_OPAQUE:
        return "PONE_OPAQUE";
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

pone_val* pone_at_key(pone_world* world, pone_val* obj, pone_val* key) {
    if (pone_type(obj) == PONE_MAP) { // specialization for performance
        return pone_map_at_key(world, obj, pone_stringify(world, key));
    } else {
        return pone_call_method(world, obj, "AT-KEY", 1, key);
    }
}
