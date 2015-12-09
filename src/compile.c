#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <dlfcn.h>
#include "pone_compile.h"
#include "pone_module.h"
#include "pone_compile.h"
#include "pone_tmpfile.h"
#include "pone_exc.h"
#include "pone_opaque.h"
#include "pone.h"
#include "pone_compile.h"
#include "pvip.h"
#include "pvip_private.h"
#include "pone_khash.h"

KHASH_SET_INIT_INT64(i64);

typedef struct pone_int_vec {
    int* a;
    size_t n; // current index
    size_t m; // sizeof a
} pone_int_vec;

static inline void pone_int_vec_init(pone_int_vec* vec) {
    vec->a = NULL;
    vec->n = 0;
    vec->m = 0;
}

static inline void pone_int_vec_push(pone_int_vec* vec, int v) {
    if (vec->n == vec->m) {
        vec->m = vec->m == 0 ? 2 : vec->m << 1;
        vec->a = realloc(vec->a, sizeof(int) * vec->m);
    }
    vec->a[vec->n++] = v;
}

static inline int pone_int_vec_last(pone_int_vec* vec) {
    return vec->a[vec->n - 1];
}

static inline void pone_int_vec_pop(pone_int_vec* vec) {
    assert(vec->n > 0);
    vec->n--;
}

typedef struct pone_compile_ctx {
    pvip_t* pvip;

    pone_world* world;

    PVIPString* buf;
    PVIPString** subs;
    int sub_idx;

    khash_t(str) * *vars_stack;
    int vars_idx;
    int vars_max;

    khash_t(i64) * ints;

    pone_int_vec loop_stack;

    pone_int_t logical_op_counter;

    bool in_class;

    pone_int_t param_no;

    pone_val* subnames;

    const char* filename;
    int anon_sub_no;
} pone_compile_ctx;

static inline void push_vars_stack(pone_compile_ctx* ctx) {
    if (ctx->vars_idx == ctx->vars_max) {
        ctx->vars_max *= 2;
        ctx->vars_stack = realloc(ctx->vars_stack, sizeof(khash_t(str)*) * ctx->vars_max);
    }

    ctx->vars_stack[ctx->vars_idx++] = kh_init(str);
}

static inline void pop_vars_stack(pone_compile_ctx* ctx) {
    kh_destroy(str, ctx->vars_stack[--ctx->vars_idx]);
}

static inline void def_lex(pone_compile_ctx* ctx, const char* name) {
    khash_t(str)* vars = ctx->vars_stack[ctx->vars_idx - 1];

    int ret;
    khint_t k = kh_put(str, vars, name, &ret);
    if (ret == -1) {
        abort(); // TODO better error msg
    }
    kh_val(vars, k) = pone_true();
}

static inline pone_node* inject_return(pone_compile_ctx* ctx, pone_node* node) {
    // printf("INJECT RETURN TO %s\n", PVIP_node_name(node->type));
    // pone_node_dump_sexp(node);
    switch (node->type) {
    case PVIP_NODE_IF:
        node->children.nodes[1] = inject_return(ctx, node->children.nodes[1]);
        if (node->children.size > 2) {
            node->children.nodes[2] = inject_return(ctx, node->children.nodes[2]);
        }
        return node;
    case PVIP_NODE_ELSE:
    case PVIP_NODE_BLOCK:
    case PVIP_NODE_STATEMENTS:
        if (PVIP_node_category(node->type) == PVIP_CATEGORY_CHILDREN) {
            if (node->children.size > 0) {
                node->children.nodes[node->children.size - 1] = inject_return(ctx, node->children.nodes[node->children.size - 1]);
            }
        }
        return node;
    case PVIP_NODE_UNDEF:
    case PVIP_NODE_RANGE:
    case PVIP_NODE_REDUCE:
    case PVIP_NODE_INT:
    case PVIP_NODE_NUMBER:
    case PVIP_NODE_DIV:
    case PVIP_NODE_MUL:
    case PVIP_NODE_ADD:
    case PVIP_NODE_SUB:
    case PVIP_NODE_IDENT:
    case PVIP_NODE_FUNCALL:
    case PVIP_NODE_STRING:
    case PVIP_NODE_BYTES:
    case PVIP_NODE_MOD:
    case PVIP_NODE_VARIABLE:
    case PVIP_NODE_MY:
    case PVIP_NODE_STRING_CONCAT:
    case PVIP_NODE_EQV:
    case PVIP_NODE_EQ:
    case PVIP_NODE_NE:
    case PVIP_NODE_LT:
    case PVIP_NODE_LE:
    case PVIP_NODE_GT:
    case PVIP_NODE_GE:
    case PVIP_NODE_ARRAY:
    case PVIP_NODE_ATPOS:
    case PVIP_NODE_METHODCALL:
    case PVIP_NODE_FUNC:
    case PVIP_NODE_NOT:
    case PVIP_NODE_CONDITIONAL:
    case PVIP_NODE_NOP:
    case PVIP_NODE_STREQ:
    case PVIP_NODE_STRNE:
    case PVIP_NODE_STRGT:
    case PVIP_NODE_STRGE:
    case PVIP_NODE_STRLT:
    case PVIP_NODE_STRLE:
    case PVIP_NODE_POW:
    case PVIP_NODE_UNARY_BOOLEAN: /* ? */
    case PVIP_NODE_UNARY_UPTO: /* ^ */
    case PVIP_NODE_STDOUT: /* $*OUT */
    case PVIP_NODE_STDERR: /* $*ERR */
    case PVIP_NODE_TWVAR: /* $*FOO */
    // DEPRECATE
    case PVIP_NODE_META_METHOD_CALL: /* $foo.^methods */
    case PVIP_NODE_REGEXP:
    case PVIP_NODE_SMART_MATCH: /* ~~ */
    case PVIP_NODE_NOT_SMART_MATCH: /* !~~ */
    case PVIP_NODE_TRUE:
    case PVIP_NODE_FALSE:
    case PVIP_NODE_HAS:
    case PVIP_NODE_ATTRIBUTE_VARIABLE: /* $!var: $.var: @.var */
    case PVIP_NODE_FUNCREF: /* &var */
    case PVIP_NODE_CMP: /* 'cmp' operator */
    case PVIP_NODE_SPECIAL_VARIABLE_REGEXP_MATCH: /* $/ - regex match */
    case PVIP_NODE_SPECIAL_VARIABLE_EXCEPTIONS: /* $@ - exceptions */
    case PVIP_NODE_SPECIAL_VARIABLE_ERRNO: /* $! - errno */
    case PVIP_NODE_SPECIAL_VARIABLE_PID: /* $$ - pid */
    case PVIP_NODE_HASH:
    case PVIP_NODE_PAIR:
    case PVIP_NODE_ATKEY:
    case PVIP_NODE_LOGICAL_AND:
    case PVIP_NODE_LOGICAL_OR:
    case PVIP_NODE_DOR: /* '//' */
    case PVIP_NODE_LAMBDA:
    case PVIP_NODE_MODULE:
    case PVIP_NODE_CLASS:
    case PVIP_NODE_METHOD:
    case PVIP_NODE_UNARY_PLUS:
    case PVIP_NODE_UNARY_MINUS:
    case PVIP_NODE_IT_METHODCALL:
    case PVIP_NODE_POSTINC:
    case PVIP_NODE_POSTDEC:
    case PVIP_NODE_PREINC:
    case PVIP_NODE_PREDEC:
    case PVIP_NODE_BRSHIFT:
    case PVIP_NODE_BLSHIFT:
    case PVIP_NODE_CHAIN:
    case PVIP_NODE_INPLACE_ADD:
    case PVIP_NODE_INPLACE_SUB:
    case PVIP_NODE_INPLACE_MUL:
    case PVIP_NODE_INPLACE_DIV:
    case PVIP_NODE_INPLACE_POW:
    case PVIP_NODE_INPLACE_MOD:
    case PVIP_NODE_INPLACE_BIN_OR:
    case PVIP_NODE_INPLACE_BIN_AND:
    case PVIP_NODE_INPLACE_BIN_XOR:
    case PVIP_NODE_INPLACE_BLSHIFT:
    case PVIP_NODE_INPLACE_BRSHIFT:
    case PVIP_NODE_INPLACE_CONCAT_S:
    case PVIP_NODE_REPEAT_S:
    case PVIP_NODE_STRINGIFY: /* prefix:<~> */
    case PVIP_NODE_NUM_CMP: /* <=> */
    case PVIP_NODE_UNICODE_CHAR: /* \c[] */
    case PVIP_NODE_BITWISE_OR: /* | */
    case PVIP_NODE_BITWISE_AND: /* & */
    case PVIP_NODE_BITWISE_XOR: /* ^ */
    case PVIP_NODE_WHATEVER: /* * */
    case PVIP_NODE_NIL: /* nil */
    case PVIP_NODE_ROLE: {
        // printf("NOT A CHILDREN %s\n", PVIP_node_name(node->type));
        pone_node* ret = pvip_node_alloc(ctx->pvip);
        ret->type = PVIP_NODE_RETURN;
        ret->children.size = 0;
        ret->children.nodes = NULL;
        ret->line_number = node->line_number;
        PVIP_node_push_child(ret, node);
        return ret;
    }
    default:
        return node;
    }
}

static inline int find_lex(pone_compile_ctx* ctx, const char* name) {
    int i = ctx->vars_idx - 1;
    while (i >= 0) {
        khash_t(str)* vars = ctx->vars_stack[i];
        khint_t k = kh_get(str, vars, name);
        if (k != kh_end(vars)) {
            return ctx->vars_idx - i - 1;
        }
        --i;
    }
    abort();
}

void _pone_compile(pone_compile_ctx* ctx, pone_node* node) {
#define PRINTF(fmt, ...) PVIP_string_printf(ctx->buf, fmt, ##__VA_ARGS__)
#define WRITE_PV(pv) PVIP_string_concat(ctx->buf, pv->buf, pv->len)
#define COMPILE(node) _pone_compile(ctx, node)
#define PUSH_SCOPE()                           \
    do {                                       \
        PRINTF("  pone_push_scope(world);\n"); \
        push_vars_stack(ctx);                  \
    } while (0)

#define POP_SCOPE()                           \
    do {                                      \
        PRINTF("  pone_pop_scope(world);\n"); \
        pop_vars_stack(ctx);                  \
    } while (0)
#define PUSH_LOOP_STACK() pone_int_vec_push(&(ctx->loop_stack), ctx->vars_idx)
#define POP_LOOP_STACK() pone_int_vec_pop(&(ctx->loop_stack))

    switch (node->type) {
    case PVIP_NODE_STATEMENTS:
        for (int i = 0; i < node->children.size; ++i) {
            pone_node* child = node->children.nodes[i];
            COMPILE(child);
            PRINTF(";\n");
        }
        break;
    case PVIP_NODE_NUMBER:
        PRINTF("pone_num_new(world, %f)", node->nv);
        break;
    case PVIP_NODE_RANGE:
        PRINTF("pone_range_new(world, ");
        COMPILE(node->children.nodes[0]);
        PRINTF(",");
        COMPILE(node->children.nodes[1]);
        PRINTF(")");
        break;
    case PVIP_NODE_INT: {
        int ret;
        (void)kh_put(i64, ctx->ints, node->iv, &ret);
        if (ret == -1) {
            fprintf(stderr, "hash operation failed\n");
            abort();
        }
        PRINTF("((pone_val*)&(pone_const_int_%ld))", node->iv);
        break;
    }
    case PVIP_NODE_REGEXP:
        PRINTF("pone_regex_new(world, \"");
        for (size_t i = 0; i < node->pv->len; ++i) {
            switch (node->pv->buf[i]) {
            case '\a':
                PRINTF("\\a");
                break;
            case '\b':
                PRINTF("\\b");
                break;
            case '\t':
                PRINTF("\\t");
                break;
            case '\n':
                PRINTF("\\n");
                break;
            case '\v':
                PRINTF("\\v");
                break;
            case '\f':
                PRINTF("\\f");
                break;
            case '\r':
                PRINTF("\\r");
                break;
            case '\\':
                PRINTF("\\\\");
                break;
            case '\"':
                PRINTF("\"");
                break;
            default:
                PRINTF("%c", node->pv->buf[i]);
                break;
            }
        }
        PRINTF("\", %ld)", node->pv->len);
        break;
    case PVIP_NODE_PAIR: {
        // (pair (ident "a") (int 3))
        pone_node* key = node->children.nodes[0];
        pone_node* value = node->children.nodes[1];
        PRINTF("pone_pair_new(world, ");
        if (key->type == PVIP_NODE_IDENT) {
            key->type = PVIP_NODE_STRING;
        }
        COMPILE(key);
        PRINTF(",");
        COMPILE(value);
        PRINTF(")");
        break;
    }
    case PVIP_NODE_BYTES:
        PRINTF("pone_bytes_new_const(world, \"");
        for (size_t i = 0; i < node->pv->len; ++i) {
            switch (node->pv->buf[i]) {
            case '\a':
                PRINTF("\\a");
                break;
            case '\b':
                PRINTF("\\b");
                break;
            case '\t':
                PRINTF("\\t");
                break;
            case '\n':
                PRINTF("\\n");
                break;
            case '\v':
                PRINTF("\\v");
                break;
            case '\f':
                PRINTF("\\f");
                break;
            case '\r':
                PRINTF("\\r");
                break;
            case '\\':
                PRINTF("\\\\");
                break;
            case '\"':
                PRINTF("\"");
                break;
            default:
                PRINTF("%c", node->pv->buf[i]);
                break;
            }
        }
        PRINTF("\", %ld)", node->pv->len);
        break;
    case PVIP_NODE_STRING:
        PRINTF("pone_str_new_const(world, \"");
        for (size_t i = 0; i < node->pv->len; ++i) {
            switch (node->pv->buf[i]) {
            case '\a':
                PRINTF("\\a");
                break;
            case '\b':
                PRINTF("\\b");
                break;
            case '\t':
                PRINTF("\\t");
                break;
            case '\n':
                PRINTF("\\n");
                break;
            case '\v':
                PRINTF("\\v");
                break;
            case '\f':
                PRINTF("\\f");
                break;
            case '\r':
                PRINTF("\\r");
                break;
            case '\\':
                PRINTF("\\\\");
                break;
            case '\"':
                PRINTF("\\\"");
                break;
            default:
                PRINTF("%c", node->pv->buf[i]);
                break;
            }
        }
        PRINTF("\", %ld)", node->pv->len);
        break;
#define SYNTAX_ERROR(msg)    \
    do {                     \
        printf("%s\n", msg); \
        abort();             \
    } while (0)
#define INPLACE(func)                                                            \
    do {                                                                         \
        if (node->children.nodes[0]->type != PVIP_NODE_VARIABLE) {               \
            SYNTAX_ERROR("left hand of inplace operator should be variable.");   \
        }                                                                        \
        int idx = find_lex(ctx, PVIP_string_c_str(node->children.nodes[0]->pv)); \
        PRINTF("%s(world, %d, \"", func, idx);                                   \
        WRITE_PV(node->children.nodes[0]->pv);                                   \
        PRINTF("\",");                                                           \
        COMPILE(node->children.nodes[1]);                                        \
        PRINTF(")");                                                             \
    } while (0)
    case PVIP_NODE_INPLACE_ADD:
        INPLACE("pone_inplace_add");
        break;
    case PVIP_NODE_INPLACE_SUB:
        INPLACE("pone_inplace_sub");
        break;
    case PVIP_NODE_INPLACE_MUL:
        INPLACE("pone_inplace_mul");
        break;
    case PVIP_NODE_INPLACE_DIV:
        INPLACE("pone_inplace_div");
        break;
    case PVIP_NODE_INPLACE_MOD:
        INPLACE("pone_inplace_mod");
        break;
    case PVIP_NODE_INPLACE_POW:
        INPLACE("pone_inplace_pow");
        break;
    case PVIP_NODE_INPLACE_BIN_OR:
        INPLACE("pone_inplace_bin_or");
        break;
    case PVIP_NODE_INPLACE_BIN_AND:
        INPLACE("pone_inplace_bin_and");
        break;
    case PVIP_NODE_INPLACE_BIN_XOR:
        INPLACE("pone_inplace_bin_xor");
        break;
    case PVIP_NODE_INPLACE_BLSHIFT:
        INPLACE("pone_inplace_blshift");
        break;
    case PVIP_NODE_INPLACE_BRSHIFT:
        INPLACE("pone_inplace_brshift");
        break;
    case PVIP_NODE_INPLACE_CONCAT_S:
        INPLACE("pone_inplace_concat_s");
        break;
#undef INPLACE
#define INFIX(func)                       \
    do {                                  \
        PRINTF("%s(world, ", func);       \
        COMPILE(node->children.nodes[0]); \
        PRINTF(",");                      \
        COMPILE(node->children.nodes[1]); \
        PRINTF(")");                      \
    } while (0)
    case PVIP_NODE_MY:
        // (my (nop) (variable "$inc"))
        def_lex(ctx, PVIP_string_c_str(node->children.nodes[1]->pv));
        PRINTF("    pone_assign(world, 0, \"");
        WRITE_PV(node->children.nodes[1]->pv);
        PRINTF("\", pone_nil());\n");
        break;
    case PVIP_NODE_ADD:
        INFIX("pone_add");
        break;
    case PVIP_NODE_SUB:
        INFIX("pone_subtract");
        break;
    case PVIP_NODE_MUL:
        INFIX("pone_multiply");
        break;
    case PVIP_NODE_DIV:
        INFIX("pone_divide");
        break;
    case PVIP_NODE_MOD:
        INFIX("pone_mod");
        break;
    case PVIP_NODE_POW:
        INFIX("pone_pow");
        break;
    case PVIP_NODE_STRING_CONCAT:
        INFIX("pone_str_concat");
        break;
    case PVIP_NODE_BITWISE_OR: /* | */
        INFIX("pone_bitwise_or");
        break;
    case PVIP_NODE_BITWISE_AND: /* & */
        INFIX("pone_bitwise_and");
        break;
    case PVIP_NODE_BITWISE_XOR: /* ^ */
        INFIX("pone_bitwise_xor");
        break;
    case PVIP_NODE_BRSHIFT: /* >> */
        INFIX("pone_brshift");
        break;
    case PVIP_NODE_BLSHIFT: /* << */
        INFIX("pone_blshift");
        break;
    case PVIP_NODE_CONDITIONAL:
        // True ? 1 : 2
        // (conditional (true) (int 1) (int 2))
        PRINTF("pone_so(");
        COMPILE(node->children.nodes[0]);
        PRINTF(") ? (");
        COMPILE(node->children.nodes[1]);
        PRINTF(") : (");
        COMPILE(node->children.nodes[2]);
        PRINTF(")");
        break;
    // See http://c-faq.com/expr/seqpoints.html
    // x or y compile to:
    // (assign(x) || assign(y), get_lex())
    case PVIP_NODE_LOGICAL_OR: {
        pone_int_t id = ctx->logical_op_counter++;
        PRINTF("(pone_so(pone_assign(world, 0, \"$<LOGICAL_OR%ld\", ", id);
        COMPILE(node->children.nodes[0]);
        PRINTF(")) || pone_assign(world, 0, \"$<LOGICAL_OR%ld\", ", id);
        COMPILE(node->children.nodes[1]);
        PRINTF("), pone_get_lex(world, \"$<LOGICAL_OR%ld\"))", id);
        break;
    }
    case PVIP_NODE_DOR: {
        pone_int_t id = ctx->logical_op_counter++;
        PRINTF("(pone_defined(pone_assign(world, 0, \"$<LOGICAL_DOR%ld\", ", id);
        COMPILE(node->children.nodes[0]);
        PRINTF(")) || pone_assign(world, 0, \"$<LOGICAL_DOR%ld\", ", id);
        COMPILE(node->children.nodes[1]);
        PRINTF("), pone_get_lex(world, \"$<LOGICAL_DOR%ld\"))", id);
        break;
    }
    case PVIP_NODE_LOGICAL_AND: {
        pone_int_t id = ctx->logical_op_counter++;
        PRINTF("(pone_so(pone_assign(world, 0, \"$<LOGICAL_AND%ld\", ", id);
        COMPILE(node->children.nodes[0]);
        PRINTF(")) && pone_assign(world, 0, \"$<LOGICAL_AND%ld\", ", id);
        COMPILE(node->children.nodes[1]);
        PRINTF("), pone_get_lex(world, \"$<LOGICAL_AND%ld\"))", id);
        break;
    }
#undef INFIX
    case PVIP_NODE_CHAIN:
        // (statements (funcall (ident "say") (args (chain (int 1) (eq (int 1))))))
        if (node->children.size > 2) {
            fprintf(stderr, "[pone] too many elements in chain: %d\n", node->children.size);
            abort();
        }

        switch (node->children.nodes[1]->type) {
#define CMP(label, func)                                     \
    case label:                                              \
        PRINTF("(" func "(world,");                          \
        COMPILE(node->children.nodes[0]);                    \
        PRINTF(",");                                         \
        COMPILE(node->children.nodes[1]->children.nodes[0]); \
        PRINTF(") ? pone_true() : pone_false())");           \
        break;

            CMP(PVIP_NODE_EQ, "pone_eq")
            CMP(PVIP_NODE_NE, "pone_ne")
            CMP(PVIP_NODE_LE, "pone_le")
            CMP(PVIP_NODE_LT, "pone_lt")
            CMP(PVIP_NODE_GT, "pone_gt")
            CMP(PVIP_NODE_GE, "pone_ge")
            CMP(PVIP_NODE_STREQ, "pone_str_eq")
            CMP(PVIP_NODE_STRNE, "pone_str_ne")
            CMP(PVIP_NODE_STRLE, "pone_str_le")
            CMP(PVIP_NODE_STRLT, "pone_str_lt")
            CMP(PVIP_NODE_STRGT, "pone_str_gt")
            CMP(PVIP_NODE_STRGE, "pone_str_ge")
        case PVIP_NODE_SMART_MATCH:
            PRINTF("pone_smart_match(world,");
            COMPILE(node->children.nodes[0]);
            PRINTF(",");
            COMPILE(node->children.nodes[1]->children.nodes[0]);
            PRINTF(")");
            break;
        default:
            fprintf(stderr, "unsupported chain node '%s'\n", PVIP_node_name(node->children.nodes[1]->type));
            abort();
        }
        break;
    case PVIP_NODE_NEXT:
        for (int i = 0; i < ctx->vars_idx - pone_int_vec_last(&(ctx->loop_stack)); i++) {
            PRINTF("  pone_pop_scope(world);\n");
        }
        PRINTF("continue;");
        break;
    case PVIP_NODE_LAST:
        for (int i = 0; i < ctx->vars_idx - pone_int_vec_last(&(ctx->loop_stack)); i++) {
            PRINTF("  pone_pop_scope(world);\n");
        }
        PRINTF("break;");
        break;
    case PVIP_NODE_ATPOS:
        // (atpos (variable "$a") (int 0))
        PRINTF("pone_at_pos(world, ");
        COMPILE(node->children.nodes[0]);
        PRINTF(",");
        COMPILE(node->children.nodes[1]);
        PRINTF(")");
        break;
    case PVIP_NODE_ATKEY:
        // (atkey (variable "$a") (int 0))
        PRINTF("pone_at_key(world, ");
        COMPILE(node->children.nodes[0]);
        PRINTF(",");
        COMPILE(node->children.nodes[1]);
        PRINTF(")");
        break;
    case PVIP_NODE_IT_METHODCALL:
        // (it_methodcall (ident "say"))
        PRINTF("pone_call_method(world, __FILE__, %d, FUNC_NAME, pone_get_lex(world, \"$_\"), \"", node->line_number);
        WRITE_PV(node->children.nodes[0]->pv);
        PRINTF("\"");
        if (node->children.size > 1) {
            PRINTF(", %d", node->children.nodes[1]->children.size);
            for (int i = 0; i < node->children.nodes[1]->children.size; ++i) {
                COMPILE(node->children.nodes[1]->children.nodes[i]);
            }
        } else {
            PRINTF(", 0");
        }
        PRINTF(")");
        break;
    case PVIP_NODE_META_METHOD_CALL:
        // (meta_method_call (string "3.14") (ident "methods") (nop))
        PRINTF("pone_call_meta_method(world, ");
        COMPILE(node->children.nodes[0]);
        PRINTF(", \"");
        WRITE_PV(node->children.nodes[1]->pv);
        if (node->children.size > 2 && node->children.nodes[2]->children.size > 0) {
            PRINTF("\", %d, ", node->children.nodes[2]->children.size);
            for (int i = 0; i < node->children.nodes[2]->children.size; ++i) {
                COMPILE(node->children.nodes[2]->children.nodes[i]);
            }
        } else {
            PRINTF("\", 0");
        }
        PRINTF(")");
        break;
    case PVIP_NODE_METHODCALL:
        // (methodcall (variable "$a") (ident "pop") (args))
        // (atpos (variable "$a") (int 0))
        PRINTF("pone_call_method(world, __FILE__, %d, FUNC_NAME, ", node->line_number);
        COMPILE(node->children.nodes[0]);
        PRINTF(", \"");
        WRITE_PV(node->children.nodes[1]->pv);
        if (node->children.size > 2) {
            PRINTF("\", %d", node->children.nodes[2]->children.size);
            for (int i = 0; i < node->children.nodes[2]->children.size; ++i) {
                PRINTF(",");
                COMPILE(node->children.nodes[2]->children.nodes[i]);
            }
        } else {
            PRINTF("\", 0");
        }
        PRINTF(")");
        break;
    case PVIP_NODE_FOR: {
        PRINTF("{\n");
        PRINTF("    pone_val* iter = pone_iter_init(world, ");
        COMPILE(node->children.nodes[0]);
        PRINTF(");\n");
        PRINTF("    while (true) {\n");
        PUSH_LOOP_STACK();
        PUSH_SCOPE();
        PRINTF("        pone_val* next = pone_iter_next(world, iter);\n");
        PRINTF("        if (next == world->universe->instance_iteration_end) {\n");
        PRINTF("            break;\n");
        PRINTF("        }\n");
        def_lex(ctx, "$_");
        PRINTF("        pone_assign(world, 0, \"$_\", next);\n");
        COMPILE(node->children.nodes[1]);
        PRINTF(";\n");
        POP_SCOPE();
        POP_LOOP_STACK();
        PRINTF("    }\n");
        PRINTF("}\n");
        break;
    }
    case PVIP_NODE_WHILE: {
        PRINTF("while (pone_so(");
        COMPILE(node->children.nodes[0]);
        PRINTF(")) {\n");
        PUSH_LOOP_STACK();
        COMPILE(node->children.nodes[1]);
        POP_LOOP_STACK();
        PRINTF("}\n");
        break;
    }
    case PVIP_NODE_NOT: {
        PRINTF("pone_not(world, ");
        COMPILE(node->children.nodes[0]);
        PRINTF(")");
        break;
    }
    case PVIP_NODE_UNARY_MINUS: // Negative numeric context operator.
        PRINTF("pone_subtract(world, pone_int_new(world, 0),");
        COMPILE(node->children.nodes[0]);
        PRINTF(")");
        break;
    case PVIP_NODE_UNARY_PLUS: // Negative numeric context operator.
        PRINTF("pone_add(world, pone_int_new(world, 0),");
        COMPILE(node->children.nodes[0]);
        PRINTF(")");
        break;
    case PVIP_NODE_TRUE:
        PRINTF("pone_true()");
        break;
    case PVIP_NODE_FALSE:
        PRINTF("pone_false()");
        break;
    case PVIP_NODE_TWVAR:
        PRINTF("pone_get_lex(world, \"");
        WRITE_PV(node->pv);
        PRINTF("\")");
        break;
    case PVIP_NODE_FUNCALL: {
        assert(
            node->children.nodes[0]->type == PVIP_NODE_IDENT
            || node->children.nodes[0]->type == PVIP_NODE_VARIABLE);
        const char* name = PVIP_string_c_str(node->children.nodes[0]->pv);
        PRINTF("pone_code_call(world, __FILE__, %d, FUNC_NAME, pone_get_lex(world, \"%s\"), pone_nil(), %d",
               node->line_number, name, node->children.nodes[1]->children.size);
        if (node->children.nodes[1]->children.size > 0) {
            PRINTF(",");
        }
        COMPILE(node->children.nodes[1]);
        PRINTF(")");
        break;
    }
    case PVIP_NODE_IF:
        // (statements (if (int 1) (statements (int 4))))
        PRINTF("if (pone_so(");
        COMPILE(node->children.nodes[0]);
        PRINTF(")) {\n");
        if (node->children.nodes[1]->type != PVIP_NODE_NOP) {
            COMPILE(node->children.nodes[1]);
        }
        PRINTF("}\n");
        for (int i = 2; i < node->children.size; ++i) {
            COMPILE(node->children.nodes[i]);
        }
        break;
    case PVIP_NODE_ELSIF:
        PRINTF(" else if (pone_so(");
        COMPILE(node->children.nodes[0]);
        PRINTF(")) {\n");
        COMPILE(node->children.nodes[1]);
        PRINTF("}\n");
        break;
    case PVIP_NODE_ELSE:
        PRINTF(" else {\n");
        for (int i = 0; i < node->children.size; ++i) {
            COMPILE(node->children.nodes[i]);
            PRINTF(";\n");
        }
        PRINTF("}\n");
        break;
    case PVIP_NODE_CLASS:
        PRINTF("{pone_val* klass=pone_class_new(world, \"");
        WRITE_PV(node->children.nodes[0]->pv);
        PRINTF("\", sizeof(\"");
        WRITE_PV(node->children.nodes[0]->pv);
        PRINTF("\")-1);");
        PRINTF("pone_assign(world, 0, \"");
        WRITE_PV(node->children.nodes[0]->pv);
        PRINTF("\", klass);");
        ctx->in_class = true;
        COMPILE(node->children.nodes[2]);
        ctx->in_class = false;
        PRINTF("}");
        break;
    case PVIP_NODE_ARRAY:
        PRINTF("pone_ary_new(world, %d",
               node->children.size);
        for (int i = 0; i < node->children.size; ++i) {
            PRINTF(",");
            COMPILE(node->children.nodes[i]);
        }
        PRINTF(")");
        break;
    case PVIP_NODE_LIST:
        PRINTF("pone_ary_new(world, %d",
               node->children.size);
        for (int i = 0; i < node->children.size; ++i) {
            PRINTF(",");
            COMPILE(node->children.nodes[i]);
        }
        PRINTF(")");
        break;
    case PVIP_NODE_HAS: {
        // has $!hoge;
        //   ==> (has (attribute_variable "$!hoge") (nop))
        break;
    }
    case PVIP_NODE_LIST_ASSIGNMENT: {
        pone_node* varnode = node->children.nodes[0];
        PVIPString* var;
        switch (varnode->type) {
        case PVIP_NODE_MY: {
            if (varnode->children.nodes[1]->type == PVIP_NODE_VARIABLE) {
                // my $n;
                var = varnode->children.nodes[1]->pv;
                def_lex(ctx, PVIP_string_c_str(var));
                int idx = find_lex(ctx, PVIP_string_c_str(var));
                PRINTF("pone_assign(world, %d, \"", idx);
                WRITE_PV(var);
                PRINTF("\", ");
                COMPILE(node->children.nodes[1]);
                PRINTF(")");
            } else if (varnode->children.nodes[1]->type == PVIP_NODE_LIST) {
                // my ($x, $y);
                pone_node* list = varnode->children.nodes[1];
                // define variables.
                for (pone_int_t i = 0; i < list->children.size; ++i) {
                    var = list->children.nodes[i]->pv;
                    def_lex(ctx, PVIP_string_c_str(var));
                }
                PRINTF("pone_assign_list(world, ");
                COMPILE(node->children.nodes[1]);
                PRINTF(", %d, ", list->children.size);
                for (pone_int_t i = 0; i < list->children.size; ++i) {
                    var = list->children.nodes[i]->pv;
                    PRINTF("\"");
                    WRITE_PV(var);
                    PRINTF("\"");
                    if (i != list->children.size - 1) {
                        PRINTF(",");
                    }
                }
                PRINTF(")");
            } else {
                pone_node_dump_sexp(varnode);
                pone_node_dump_sexp(node);
                abort();
            }
            break;
        }
        case PVIP_NODE_VARIABLE: {
            var = varnode->pv;
            int idx = find_lex(ctx, PVIP_string_c_str(var));
            PRINTF("pone_assign(world, %d, \"", idx);
            WRITE_PV(var);
            PRINTF("\", ");
            COMPILE(node->children.nodes[1]);
            PRINTF(")");
            break;
        }
        case PVIP_NODE_ATPOS: {
            pone_node* var = varnode->children.nodes[0];
            pone_node* pos = varnode->children.nodes[1];
            PRINTF("pone_assign_pos(world, pone_get_lex(world, \"");
            WRITE_PV(var->pv);
            PRINTF("\"), ");
            COMPILE(pos);
            PRINTF(", ");
            COMPILE(node->children.nodes[1]);
            PRINTF(")");
            break;
        }
        case PVIP_NODE_ATKEY: {
            pone_node* var = varnode->children.nodes[0];
            pone_node* key = varnode->children.nodes[1];
            PRINTF("pone_assign_key(world, pone_get_lex(world, \"");
            WRITE_PV(var->pv);
            PRINTF("\"), ");
            COMPILE(key);
            PRINTF(", ");
            COMPILE(node->children.nodes[1]);
            PRINTF(")");
            break;
        }
        case PVIP_NODE_ATTRIBUTE_VARIABLE: {
            // $!hoge = 3;
            // // (list_assignment (attribute_variable "$!hoge") (int 3))
            PRINTF("pone_obj_set_ivar(world, self, \"");
            WRITE_PV(varnode->pv);
            PRINTF("\", ");
            COMPILE(node->children.nodes[1]);
            PRINTF(")");
            break;
        }
        default:
            fprintf(stderr, "invalid node at lhs at line %d, %s\n",
                    node->line_number, PVIP_node_name(varnode->type));
            abort();
        }
        break;
    }
    case PVIP_NODE_ATTRIBUTE_VARIABLE: {
        PRINTF("pone_obj_get_ivar(world, self, \"");
        WRITE_PV(node->pv);
        PRINTF("\")");
        break;
    }
    case PVIP_NODE_IDENT: {
        if (node->pv->len == 4 && memcmp(node->pv->buf, "self", 4) == 0) {
            PRINTF("(self)");
        } else {
            PRINTF("pone_get_lex(world, \"");
            WRITE_PV(node->pv);
            PRINTF("\")");
        }
        break;
    }
    case PVIP_NODE_VARIABLE: {
        PRINTF("pone_get_lex(world, \"");
        WRITE_PV(node->pv);
        PRINTF("\")");
        break;
    }
    case PVIP_NODE_ARGS:
        for (int i = 0; i < node->children.size; ++i) {
            pone_node* child = node->children.nodes[i];
            COMPILE(child);
            if (i != node->children.size - 1) {
                PRINTF(",");
            }
        }
        break;
    case PVIP_NODE_HASH:
        // (statements (hash (pair (ident "a") (int 3))))
        PRINTF("pone_map_assign_keys(world, pone_map_new(world), %d", node->children.size * 2);
        for (int i = 0; i < node->children.size; ++i) {
            PRINTF(",");

            pone_node* child = node->children.nodes[i];
            if (child->children.nodes[0]->type == PVIP_NODE_IDENT) {
                WRITE_PV(child->children.nodes[0]->pv);
            } else {
                COMPILE(child->children.nodes[0]);
            }
            PRINTF(",");
            COMPILE(child->children.nodes[1]);
        }
        PRINTF(")");
        break;
    case PVIP_NODE_RETURN: {
        PRINTF("return ");
        COMPILE(node->children.nodes[0]);
        PRINTF(";\n");
        break;
    }
    case PVIP_NODE_SPECIAL_VARIABLE_EXCEPTIONS: { // $@
        PRINTF("pone_errvar(world)");
        break;
    }
    case PVIP_NODE_SPECIAL_VARIABLE_ERRNO: { // $!
        PRINTF("pone_errno(world)");
        break;
    }
    case PVIP_NODE_SPECIAL_VARIABLE_PID: { // $$
        PRINTF("pone_int_new(world, getpid())");
        break;
    }
    case PVIP_NODE_BLOCK: {
        PRINTF("{\n");
        PUSH_SCOPE();
        if (node->children.size > 0) {
            COMPILE(node->children.nodes[0]);
        }
        POP_SCOPE();
        PRINTF("}\n");
        break;
    }
    case PVIP_NODE_TRY: {
        ctx->anon_sub_no++;

#define SAVE_BUF                                                              \
    PVIPString* orig_buf = ctx->buf;                                          \
    ctx->subs = realloc(ctx->subs, sizeof(PVIPString*) * (ctx->sub_idx + 1)); \
    ctx->buf = PVIP_string_new();                                             \
    ctx->subs[ctx->sub_idx++] = ctx->buf;

#define RESTORE_BUF ctx->buf = orig_buf

        SAVE_BUF;

        PRINTF("pone_val* pone_try_%d(", ctx->anon_sub_no);
        PRINTF("pone_world* world, pone_val* self, int n, va_list args) {\n");
        PUSH_SCOPE();
        PRINTF("  const char* FUNC_NAME=\"<try>\";\n");
        COMPILE(inject_return(ctx, node->children.nodes[0]));
        POP_SCOPE();
        PRINTF("  return pone_nil();\n");
        PRINTF("}\n");

        RESTORE_BUF;

        PRINTF("pone_try(world, pone_code_new(world, pone_try_%d))",
               ctx->anon_sub_no);

        break;
    }
    case PVIP_NODE_FUNC: {
        // (func (ident "x") (params) (nop) (block (statements (funcall (ident "say") (args (int 3))))))
        // (func (ident "x") (params (param (nop) (variable "$n") (nop) (int 0))) (nop) (block (statements (funcall (ident "say") (args (variable "$n"))))))
        pone_node* name = node->children.nodes[0];
        // int argcnt = node->children.nodes[1]->children.size;

        bool is_anon = false;
        if (name->type == PVIP_NODE_NOP) {
            is_anon = true;
            ctx->anon_sub_no++;
        }

        SAVE_BUF;

        PRINTF("pone_val* pone_user_func_");
        if (is_anon) {
            pone_ary_push(ctx->world, ctx->subnames, pone_str_new_printf(ctx->world, "pone_user_func_anon_%d", ctx->anon_sub_no));
            PRINTF("anon_%d", ctx->anon_sub_no);
        } else {
            pone_ary_push(ctx->world, ctx->subnames, pone_str_new_printf(ctx->world, "pone_user_func_%s", PVIP_string_c_str(name->pv)));
            WRITE_PV(name->pv);
        }
        PRINTF("(pone_world* world, pone_val* self, int n, va_list args) {\n");
        PUSH_SCOPE();
        if (is_anon) {
            PRINTF("    const char* FUNC_NAME=\"<ANON>\";\n");
        } else {
            PRINTF("    const char* FUNC_NAME=\"");
            WRITE_PV(name->pv);
            PRINTF("\";\n");
        }
        COMPILE(node->children.nodes[1]);
        COMPILE(inject_return(ctx, node->children.nodes[3]));
        POP_SCOPE();
        PRINTF("  return pone_nil();\n");
        PRINTF("}\n");

        RESTORE_BUF;

        if (is_anon) {
            PRINTF("pone_code_new(world, pone_user_func_anon_%d)",
                   ctx->anon_sub_no);
        } else if (ctx->in_class) {
            PRINTF("pone_add_method(world, klass, \"%s\", strlen(\"%s\"), pone_code_new_c(world, pone_user_func_%s))",
                   PVIP_string_c_str(name->pv),
                   PVIP_string_c_str(name->pv),
                   PVIP_string_c_str(name->pv));
        } else {
            PRINTF("pone_assign(world, 0, \"%s\", pone_code_new(world, pone_user_func_%s))",
                   PVIP_string_c_str(name->pv),
                   PVIP_string_c_str(name->pv));
        }

        break;
    }
    case PVIP_NODE_PARAM: {
        // (param (nop) (variable "$n") (nop) (int 0))
        // (param (nop) (list) (nop) (int 0))
        // (param (nop) (variable "$m") (int 3) (int 0))
        if (node->children.nodes[1]->type == PVIP_NODE_VARIABLE) {
            // sub x($x=1) { }; x(1) => n=1, param_no=0
            // sub x($x=1) { }; x()  => n=0, param_no=0
            PRINTF("if (n>%d) {", ctx->param_no);
            {
                PRINTF("pone_assign(world, 0, \"");
                WRITE_PV(node->children.nodes[1]->pv);
                PRINTF("\", ");
                PRINTF("va_arg(args, pone_val*)");
                PRINTF(");\n");
            }
            PRINTF("} else {");
            {
                // use default argument.
                PRINTF("pone_assign(world, 0, \"");
                WRITE_PV(node->children.nodes[1]->pv);
                PRINTF("\", ");
                COMPILE(node->children.nodes[2]);
                PRINTF(");\n");
            }
            PRINTF("}");
        } else if (node->children.nodes[1]->type == PVIP_NODE_LIST) {
            // ignore this.
            // sub () { }
        } else {
            pone_node_dump_sexp(node);
            abort();
        }
        break;
    }
    case PVIP_NODE_PARAMS: {
        pone_int_t min_params = 0;
        pone_int_t max_params = node->children.size;
        for (pone_int_t i = 0; i < node->children.size; ++i) {
            pone_node* var_node = node->children.nodes[i]->children.nodes[1];
            pone_node* default_node = node->children.nodes[i]->children.nodes[2];
            if (var_node->type == PVIP_NODE_VARIABLE && default_node->type == PVIP_NODE_NOP) {
                min_params++;
            }
        }
        PRINTF("if (n<%d) { pone_throw_str(world, \"Expect %d..%d parameters but you passed %%d.\", n); }", min_params, min_params, max_params);
        for (pone_int_t i = 0; i < node->children.size; ++i) {
            ctx->param_no = i;
            COMPILE(node->children.nodes[i]);
        }
        break;
    }
    case PVIP_NODE_USE: {
        // (use (ident "io\/socket\/inet"))
        // (use (nop) (ident "socket"))
        PRINTF("pone_use(world, \"");
        WRITE_PV(node->children.nodes[1]->pv);
        PRINTF("\",");
        if (node->children.nodes[0]->type == PVIP_NODE_NOP) {
            const char* p = PVIP_string_c_str(node->children.nodes[1]->pv);
            const char* s = strrchr(p, '/');
            if (s) {
                s++;
            } else {
                s = p;
            }
            PRINTF("\"%s\", sizeof(\"%s\")-1", s, s);
        } else {
            PRINTF("\"");
            WRITE_PV(node->children.nodes[0]->pv);
            PRINTF("\", sizeof(\"");
            WRITE_PV(node->children.nodes[0]->pv);
            PRINTF("\")-1");
        }
        PRINTF(")");
        break;
    }
    case PVIP_NODE_NOP:
        PRINTF("pone_nil()");
        break;
    case PVIP_NODE_NIL:
        PRINTF("pone_nil()");
        break;
    default:
        fprintf(stderr, "unsupported node '%s'\n", PVIP_node_name(node->type));
        abort();
    }
#undef PRINTF
#undef COMPILE
}

void pone_compile(pone_compile_ctx* ctx, FILE* fp, pone_node* node, const char* so_funcname, const char* module_name) {
    _pone_compile(ctx, node);

#define PRINTF(fmt, ...) fprintf(fp, (fmt), ##__VA_ARGS__)
    PRINTF("#include \"pone.h\"\n");
    PRINTF("#include \"pone_module.h\"\n");
    PRINTF("#include \"pone_exc.h\"\n");
    PRINTF("\n");
    PRINTF("// --------------- vvvv ints          vvvv -------------------\n");
    {
        khint_t i;
        for (i = kh_begin(ctx->ints); i != kh_end(ctx->ints); ++i) {
            if (!kh_exist(ctx->ints, i))
                continue;
            pone_int_t key = kh_key(ctx->ints, i);
            PRINTF("static pone_int pone_const_int_%ld = { .type=PONE_INT, .flags=PONE_FLAGS_FROZEN, .i=%ld };\n", key, key);
        }
    }
    PRINTF("// --------------- vvvv function prototypes    vvvv -------------------\n");
    for (int i = 0; i < pone_ary_size(ctx->subnames); ++i) {
        pone_val* v = pone_ary_at_pos(ctx->subnames, i);
        PRINTF("pone_val* ");
        fwrite(pone_str_ptr(v), 1, pone_str_len(v), fp);
        PRINTF("(pone_world* world, pone_val* self, int n, va_list args);\n");
    }
    PRINTF("// --------------- vvvv functions     vvvv -------------------\n");
    for (int i = 0; i < ctx->sub_idx; ++i) {
        fwrite(ctx->subs[i]->buf, 1, ctx->subs[i]->len, fp);
    }
    PRINTF("// --------------- vvvv loader point vvvv -------------------\n");
    PRINTF("pone_val* %s(pone_world* world, pone_val* self, int n, va_list args) {\n", so_funcname);
    PRINTF("    const char* FUNC_NAME=\"%s\";\n", module_name);
    fwrite(ctx->buf->buf, 1, ctx->buf->len, fp);
    PRINTF("    return pone_module_from_lex(world, \"%s\");\n", module_name);
    PRINTF("}\n");
}

pone_val* pone_load_dll(pone_world* world, const char* so_path, const char* funcname) {
    void* handle = dlopen(so_path, RTLD_LAZY);
    if (!handle) {
        pone_throw_str(world, "dlopen: %s", dlerror());
    }
    pone_funcptr_t pone_so_init = dlsym(handle, funcname);

    char* error;
    if ((error = dlerror()) != NULL) {
        pone_throw_str(world, "dlerror: %s", error);
    }

    pone_val* code = pone_code_new(world, pone_so_init);
    code->as.code.lex = pone_lex_new(world, NULL);
    return code;
}

static void pone_compile_ctx_finalize(pone_world* world, pone_val* val) {
    pone_compile_ctx* ctx = pone_opaque_ptr(val);
    for (int i = 0; i < ctx->sub_idx; ++i) {
        PVIP_string_destroy(ctx->subs[i]);
    }
    if (ctx->subs) {
        free(ctx->subs);
    }
    kh_destroy(i64, ctx->ints);
    free(ctx->vars_stack);
    PVIP_string_destroy(ctx->buf);
    pvip_free(ctx->pvip);
    pone_free(world, ctx);
}

static pone_compile_ctx* pone_compile_ctx_new(pone_world* world, const char* filename) {
    assert(world);

    pone_compile_ctx* ctx = pone_malloc(world, sizeof(pone_compile_ctx));
    assert(ctx);

    ctx->world = world;

    ctx->pvip = pvip_new();
    ctx->buf = PVIP_string_new();

    ctx->subs = NULL;
    ctx->sub_idx = 0;

    ctx->vars_stack = pone_malloc(world, sizeof(khash_t(str)*) * 1);
    ctx->vars_stack[0] = kh_init(str);
    ctx->vars_max = 1;
    ctx->vars_idx = 0;

    ctx->ints = kh_init(i64);

    pone_int_vec_init(&(ctx->loop_stack));

    ctx->logical_op_counter = 0;

    ctx->in_class = false;

    ctx->filename = filename;

    ctx->anon_sub_no = 0;

    ctx->subnames = pone_ary_new(world, 0);

    // managed by GC
    (void)pone_opaque_new(world, pone_class_new(world, "CompilerContext", strlen("CompilerContext")), ctx, pone_compile_ctx_finalize);
    return ctx;
}

static const char* gen_tmpfile(pone_world* world) {
    pone_val* c_tmpfile_v = pone_tmpfile_new(world);
    return pone_tmpfile_path_c(world, c_tmpfile_v);
}

static void pone_compile_node(pone_world* world, pone_node* node, const char* filename, pone_compile_ctx* ctx, const char* c_filename, const char* so_filename, const char* so_funcname) {

    FILE* fp = fopen(c_filename, "w");
    if (!fp) {
        pone_throw_str(world, "cannot open generated C source file: %s", c_filename);
    }

    push_vars_stack(ctx);
    pone_compile(ctx, fp, node, so_funcname, "main");
    pop_vars_stack(ctx);

    fclose(fp);

    pone_compile_c(world, so_filename, c_filename);
}

pone_val* pone_compile_fp(pone_world* world, FILE* srcfp, const char* filename) {
    assert(world);
    pone_compile_ctx* ctx = pone_compile_ctx_new(world, filename);
    pone_node* node = pone_parse_fp(world, ctx->pvip, srcfp, false);
    const char* c_filename = pone_str_ptr(pone_str_c_str(world, pone_str_new_printf(world, "%s.c", filename)));
    const char* so_filename = pone_str_ptr(pone_str_c_str(world, pone_str_new_printf(world, "%s.so", filename)));
    const char* so_funcname = "pone_so_init_0";
    pone_compile_node(world, node, filename, ctx, c_filename, so_filename, so_funcname);
    pone_val* code = pone_load_dll(world, so_filename, so_funcname);
    return code;
}

pone_val* pone_compile_str(pone_world* world, const char* src) {
    assert(world);
    pone_compile_ctx* ctx = pone_compile_ctx_new(world, "-e");
    pone_node* node = pone_parse_string(world, ctx->pvip, src, false);
    const char* c_filename = gen_tmpfile(world);
    const char* so_filename = gen_tmpfile(world);
    const char* so_funcname = "pone_so_init_0";
    pone_compile_node(world, node, "-e", ctx, c_filename, so_filename, so_funcname);
    pone_val* code = pone_load_dll(world, so_filename, so_funcname);
    return code;
}

pone_node* pone_parse_string(pone_world* world, pvip_t* pvip, const char* src, bool yy_debug) {
    PVIPString* error;
    pone_node* node = PVIP_parse_string(pvip, src, strlen(src), yy_debug, &error);
    if (!node) {
        pone_val* s = pone_str_new_strdup(world, "Cannot parse string: ",
                                          strlen("Cannot parse string: "));
        pone_str_append_c(world, s, error->buf, error->len);
        PVIP_string_destroy(error);
        pone_throw(world, s);
    }
    return node;
}

pone_node* pone_parse_fp(pone_world* world, pvip_t* pvip, FILE* fp, bool yy_debug) {
    PVIPString* error;
    pone_node* node = PVIP_parse_fp(pvip, fp, yy_debug, &error);
    if (!node) {
        pone_val* s = pone_str_new_strdup(world, "Cannot parse string: ",
                                          strlen("Cannot parse: "));
        pone_str_append_c(world, s, error->buf, error->len);
        PVIP_string_destroy(error);
        pone_throw(world, s);
    }
    return node;
}

void pone_compile_c(pone_world* world, const char* so_filename, const char* c_filename) {
    const char* include_path = getenv("PONE_INCLUDE"); // should we cache this value?
    if (!include_path) {
        include_path = PONE_INCLUDE_DIR;
    }
    pone_val* cmdline = pone_str_c_str(world, pone_str_new_printf(world, "clang -x c -D_POSIX_C_SOURCE=200809L -rdynamic -fPIC -shared -I%s -I src/ -g -lm -std=c99 -o %s %s -L. -lpone", include_path, so_filename, c_filename));
    int retval = system(pone_str_ptr(cmdline));
    if (retval != 0) {
        pone_throw_str(world, "cannot compile generated C code");
    }
}
