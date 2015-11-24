#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <dlfcn.h>
#include "pvip.h"
#include "pvip_private.h"
#include "pone.h"
#include "linenoise.h"

#ifndef CC
#define CC cc
#endif

KHASH_SET_INIT_INT64(i64);

static void usage() {
    printf("Usage: pone -e=code\n"
            "    pone src.pone\n");
}

typedef struct pone_int_vec {
    int *a;
    size_t n; // current index
    size_t m; // sizeof a
} pone_int_vec;

static inline void pone_int_vec_init(pone_int_vec* vec) {
    vec->a = NULL;
    vec->n = 0;
    vec->m = 0;
}

static inline void pone_int_vec_push(pone_int_vec* vec, int v) {
    if (vec->n==vec->m) {
        vec->m = vec->m == 0 ? 2 : vec->m<<1;
        vec->a = realloc(vec->a, sizeof(int)*vec->m);
    }
    vec->a[vec->n++] = v;
}

static inline int pone_int_vec_last(pone_int_vec* vec) {
    return vec->a[vec->n-1];
}

static inline void pone_int_vec_pop(pone_int_vec* vec) {
    assert(vec->n > 0);
    vec->n--;
}

typedef struct pone_compile_ctx {
    pvip_t* pvip;

    PVIPString* buf;
    PVIPString** subs;
    int sub_idx;

    khash_t(str) **vars_stack;
    int vars_idx;
    int vars_max;

    khash_t(i64) *ints;

    pone_int_vec loop_stack;

    pone_int_t logical_op_counter;

    const char* filename;
    int anon_sub_no;
} pone_compile_ctx;

static inline void push_vars_stack(pone_compile_ctx* ctx) {
    if (ctx->vars_idx == ctx->vars_max) {
        ctx->vars_max *= 2;
        ctx->vars_stack = realloc(ctx->vars_stack, sizeof(khash_t(str)*)*ctx->vars_max);
    }

    ctx->vars_stack[ctx->vars_idx++] = kh_init(str);
}

static inline void pop_vars_stack(pone_compile_ctx* ctx) {
    kh_destroy(str, ctx->vars_stack[--ctx->vars_idx]);
}

static inline void def_lex(pone_compile_ctx* ctx, const char* name) {
    khash_t(str)* vars = ctx->vars_stack[ctx->vars_idx-1];

    int ret;
    khint_t k = kh_put(str, vars, name, &ret);
    if (ret == -1) {
        abort(); // TODO better error msg
    }
    kh_val(vars, k) = pone_true();
}

static inline pone_node* inject_return(pone_compile_ctx* ctx, pone_node* node) {
    // printf("INJECT RETURN TO %s\n", PVIP_node_name(node->type));
    // PVIP_node_dump_sexp(node);
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
                node->children.nodes[node->children.size-1] = inject_return(ctx, node->children.nodes[node->children.size-1]);
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
    // DEPRECATE
    case PVIP_NODE_TW_INC: /* @*INC */
    case PVIP_NODE_META_METHOD_CALL: /* $foo.^methods */
    case PVIP_NODE_REGEXP:
    case PVIP_NODE_SMART_MATCH: /* ~~ */
    case PVIP_NODE_NOT_SMART_MATCH: /* !~~ */
    case PVIP_NODE_TRUE:
    case PVIP_NODE_FALSE:
    case PVIP_NODE_HAS:
    case PVIP_NODE_ATTRIBUTE_VARIABLE:  /* $!var: $.var: @.var */
    case PVIP_NODE_FUNCREF:           /* &var */
    case PVIP_NODE_TW_PACKAGE: /* $?PACKAGE */
    case PVIP_NODE_TW_CLASS: /* $?CLASS */
    // DEPRECATE
    case PVIP_NODE_TW_MODULE: /* $?MODULE */
    case PVIP_NODE_TW_PID: /* $*PID */
    case PVIP_NODE_TW_PERLVER: /* $*PPERLVER */
    case PVIP_NODE_TW_EXECUTABLE_NAME: /* $*EXECUTABLE_NAME */
    case PVIP_NODE_TW_ROUTINE: /* &?ROUTINE */
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
    case PVIP_NODE_BITWISE_OR:  /* | */
    case PVIP_NODE_BITWISE_AND: /* & */
    case PVIP_NODE_BITWISE_XOR: /* ^ */
    case PVIP_NODE_WHATEVER: /* * */
    case PVIP_NODE_ROLE: {
        // printf("NOT A CHILDREN %s\n", PVIP_node_name(node->type));
        pone_node *ret = pvip_node_alloc(ctx->pvip);
        ret->type = PVIP_NODE_RETURN;
        ret->children.size  = 0;
        ret->children.nodes = NULL;
        PVIP_node_push_child(ret, node);
        return ret;
    }
    default:
            return node;
    }
}

static inline int find_lex(pone_compile_ctx* ctx, const char* name) {
    int i = ctx->vars_idx-1;
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
#define PRINTF(fmt, ...) PVIP_string_printf(ctx->buf, fmt,  ##__VA_ARGS__)
#define WRITE_PV(pv) PVIP_string_concat(ctx->buf, pv->buf, pv->len)
#define LINE(node) PRINTF("#line %d \"%s\"\n", (node)->line_number, ctx->filename);
#define COMPILE(node) _pone_compile(ctx, node)
#define PUSH_SCOPE() \
    do { \
        PRINTF("  pone_push_scope(world);\n"); \
        push_vars_stack(ctx); \
    } while (0)

#define POP_SCOPE() \
    do { \
        PRINTF("  pone_pop_scope(world);\n"); \
        pop_vars_stack(ctx); \
    } while (0)
#define PUSH_LOOP_STACK() pone_int_vec_push(&(ctx->loop_stack), ctx->vars_idx)
#define POP_LOOP_STACK() pone_int_vec_pop(&(ctx->loop_stack))

    switch (node->type) {
        case PVIP_NODE_STATEMENTS:
            for (int i=0; i<node->children.size; ++i) {
                pone_node* child = node->children.nodes[i];
                int n = (child)->line_number;
                const char* filename = ctx->filename;
                PRINTF("#line %d \"%s\"\n", n, filename);
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
            (void) kh_put(i64, ctx->ints, node->iv, &ret);
            if (ret == -1) {
                fprintf(stderr, "hash operation failed\n");
                abort();
            }
            PRINTF("((pone_val*)&(pone_const_int_%ld))", node->iv);
            break;
        }
        case PVIP_NODE_REGEXP:
            PRINTF("pone_regex_new(world, \"");
            for (size_t i=0; i<node->pv->len; ++i) {
                switch (node->pv->buf[i]) {
                case '\a': PRINTF("\\a"); break;
                case '\b': PRINTF("\\b"); break;
                case '\t': PRINTF("\\t"); break;
                case '\n': PRINTF("\\n"); break;
                case '\v': PRINTF("\\v"); break;
                case '\f': PRINTF("\\f"); break;
                case '\r': PRINTF("\\r"); break;
                case '\\': PRINTF("\\\\"); break;
                case '\"': PRINTF("\""); break;
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
            for (size_t i=0; i<node->pv->len; ++i) {
                switch (node->pv->buf[i]) {
                case '\a': PRINTF("\\a"); break;
                case '\b': PRINTF("\\b"); break;
                case '\t': PRINTF("\\t"); break;
                case '\n': PRINTF("\\n"); break;
                case '\v': PRINTF("\\v"); break;
                case '\f': PRINTF("\\f"); break;
                case '\r': PRINTF("\\r"); break;
                case '\\': PRINTF("\\\\"); break;
                case '\"': PRINTF("\""); break;
                default:
                    PRINTF("%c", node->pv->buf[i]);
                    break;
                }
            }
            PRINTF("\", %ld)", node->pv->len);
            break;
        case PVIP_NODE_STRING:
            PRINTF("pone_str_new_const(world, \"");
            for (size_t i=0; i<node->pv->len; ++i) {
                switch (node->pv->buf[i]) {
                case '\a': PRINTF("\\a"); break;
                case '\b': PRINTF("\\b"); break;
                case '\t': PRINTF("\\t"); break;
                case '\n': PRINTF("\\n"); break;
                case '\v': PRINTF("\\v"); break;
                case '\f': PRINTF("\\f"); break;
                case '\r': PRINTF("\\r"); break;
                case '\\': PRINTF("\\\\"); break;
                case '\"': PRINTF("\""); break;
                default:
                    PRINTF("%c", node->pv->buf[i]);
                    break;
                }
            }
            PRINTF("\", %ld)", node->pv->len);
            break;
#define SYNTAX_ERROR(msg) do { printf("%s\n", msg); abort(); } while (0)
#define INPLACE(func) do { \
        if (node->children.nodes[0]->type != PVIP_NODE_VARIABLE) { \
            SYNTAX_ERROR("left hand of inplace operator should be variable."); \
        } \
        int idx = find_lex(ctx, PVIP_string_c_str(node->children.nodes[0]->pv)); \
        PRINTF("%s(world, %d, \"", func, idx); \
        WRITE_PV(node->children.nodes[0]->pv); \
        PRINTF("\","); \
        COMPILE(node->children.nodes[1]); \
        PRINTF(")"); \
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
#define INFIX(func) do { PRINTF("%s(world, ", func); COMPILE(node->children.nodes[0]);  PRINTF(","); COMPILE(node->children.nodes[1]); PRINTF(")"); } while (0)
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
        case PVIP_NODE_BITWISE_OR:  /* | */
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
#define CMP(label, func) \
                case label: \
                    PRINTF("(" func "(world,"); \
                    COMPILE(node->children.nodes[0]); \
                    PRINTF(","); \
                    COMPILE(node->children.nodes[1]->children.nodes[0]); \
                    PRINTF(") ? pone_true() : pone_false())"); \
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
            for (int i=0; i<ctx->vars_idx-pone_int_vec_last(&(ctx->loop_stack)); i++) {
                PRINTF("  pone_pop_scope(world);\n"); \
            }
            PRINTF("continue;");
            break;
        case PVIP_NODE_LAST:
            for (int i=0; i<ctx->vars_idx-pone_int_vec_last(&(ctx->loop_stack)); i++) {
                PRINTF("  pone_pop_scope(world);\n"); \
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
            PRINTF("pone_call_method(world, pone_get_lex(world, \"$_\"), \"");
            WRITE_PV(node->children.nodes[0]->pv);
            PRINTF("\"");
            if (node->children.size > 1) {
                PRINTF(", %d", node->children.nodes[1]->children.size);
                for (int i=0; i<node->children.nodes[1]->children.size; ++i) {
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
            if (node->children.size > 2) {
                PRINTF("\", %d", node->children.nodes[2]->children.size);
                for (int i=0; i<node->children.nodes[2]->children.size; ++i) {
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
            PRINTF("pone_call_method(world, ");
            COMPILE(node->children.nodes[0]);
            PRINTF(", \"");
            WRITE_PV(node->children.nodes[1]->pv);
            if (node->children.size > 2) {
                PRINTF("\", %d", node->children.nodes[2]->children.size);
                for (int i=0; i<node->children.nodes[2]->children.size; ++i) {
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
        case PVIP_NODE_FUNCALL: {
            assert(
                    node->children.nodes[0]->type == PVIP_NODE_IDENT
                    || node->children.nodes[0]->type == PVIP_NODE_VARIABLE
                    );
            const char* name = PVIP_string_c_str(node->children.nodes[0]->pv);
            PRINTF("pone_code_call(world, pone_get_lex(world, \"%s\"), pone_nil(), %d",
                    name, node->children.nodes[1]->children.size);
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
            COMPILE(node->children.nodes[1]);
            PRINTF("}\n");
            for (int i=2; i<node->children.size; ++i) {
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
            for (int i=0; i<node->children.size; ++i) {
                LINE(node->children.nodes[i]);
                COMPILE(node->children.nodes[i]);
                PRINTF(";\n");
            }
            PRINTF("}\n");
            break;
        case PVIP_NODE_ARRAY:
            PRINTF("pone_ary_new(world, %d",
                    node->children.size);
            for (int i=0; i<node->children.size; ++i) {
                PRINTF(",");
                COMPILE(node->children.nodes[i]);
            }
            PRINTF(")");
            break;
        case PVIP_NODE_LIST:
            PRINTF("pone_ary_new(world, %d",
                    node->children.size);
            for (int i=0; i<node->children.size; ++i) {
                PRINTF(",");
                COMPILE(node->children.nodes[i]);
            }
            PRINTF(")");
            break;
        case PVIP_NODE_LIST_ASSIGNMENT: {
            pone_node* varnode = node->children.nodes[0];
            PVIPString *var;
            switch (varnode->type) {
            case PVIP_NODE_MY: {
                var = varnode->children.nodes[1]->pv;
                def_lex(ctx, PVIP_string_c_str(var));
                int idx = find_lex(ctx, PVIP_string_c_str(var));
                PRINTF("pone_assign(world, %d, \"", idx);
                WRITE_PV(var);
                PRINTF("\", ");
                COMPILE(node->children.nodes[1]);
                PRINTF(")");
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
            default:
                fprintf(stderr, "invalid node at lhs at line %d, %s\n",
                        node->line_number, PVIP_node_name(varnode->type));
                abort();
            }
            break;
        }
        case PVIP_NODE_IDENT: {
            PRINTF("pone_get_lex(world, \"");
            WRITE_PV(node->pv);
            PRINTF("\")");
            break;
        }
        case PVIP_NODE_VARIABLE: {
            PRINTF("pone_get_lex(world, \"");
            WRITE_PV(node->pv);
            PRINTF("\")");
            break;
        }
        case PVIP_NODE_ARGS:
            for (int i=0; i<node->children.size; ++i) {
                pone_node* child = node->children.nodes[i];
                COMPILE(child);
                if (i!=node->children.size-1) {
                    PRINTF(",");
                }
            }
            break;
        case PVIP_NODE_HASH:
            // (statements (hash (pair (ident "a") (int 3))))
            PRINTF("pone_hash_assign_keys(world, pone_hash_new(world), %d", node->children.size*2);
            for (int i=0; i<node->children.size; ++i) {
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

#define SAVE_BUF \
        PVIPString* orig_buf = ctx->buf; \
        ctx->subs = realloc(ctx->subs, sizeof(PVIPString*)*(ctx->sub_idx+1)); \
        ctx->buf = PVIP_string_new(); \
        ctx->subs[ctx->sub_idx++] = ctx->buf;

#define RESTORE_BUF ctx->buf = orig_buf

            SAVE_BUF;

            PRINTF("pone_val* pone_try_%d(", ctx->anon_sub_no);
            PRINTF("pone_world* world, pone_val* self, int n, va_list args) {\n");
            PUSH_SCOPE();
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
                PRINTF("anon_%d", ctx->anon_sub_no);
            } else {
                WRITE_PV(name->pv);
            }
            PRINTF("(pone_world* world, pone_val* self, int n, va_list args) {\n");
            PUSH_SCOPE();
            COMPILE(node->children.nodes[1]);
            COMPILE(inject_return(ctx, node->children.nodes[3]));
            POP_SCOPE();
            PRINTF("  return pone_nil();\n");
            PRINTF("}\n");

            RESTORE_BUF;

            if (is_anon) {
                PRINTF("pone_code_new(world, pone_user_func_anon_%d)",
                        ctx->anon_sub_no);
            } else {
                PRINTF("pone_assign(world, 0, \"%s\", pone_code_new(world, pone_user_func_%s))",
                        PVIP_string_c_str(name->pv),
                        PVIP_string_c_str(name->pv));
            }

//      if .name {
//          qq!pone_assign(world, 0, "&{$name}", pone_mortalize(world, pone_code_new(world, pone_user_func_{$name})))!;
//      } else {
//          qq!pone_mortalize(world, pone_code_new(world, pone_user_func_{$name}))!;
//      }
            /*
        my $name = .name // 'anon_' ~ $*ANONSUBNO++;
        my $argcnt = .children[1] ?? .children[1].children.elems !! 0;
        my $s = '';
        $s ~= sprintf(qq!#line %d "%s"\n!, .lineno, $!filename);
        $s ~= 'pone_val* pone_user_func_';
        $s ~= $name;
        $s ~= '(pone_world* world, pone_val* self, int n, va_list args) {' ~ "\n";
        $s ~= "assert(n==$argcnt);\n";
        @*TMPS.push(0);
        @*SCOPE.push(0);
        $s ~= "pone_push_scope(world);\n";
        # bind parameters to lexical variables
        if $argcnt > 0 {
            if .children[1] {
                for 0..^(.children[1].children.elems) -> $i {
                    my $var = .children[1].children[$i];
                    $s ~= "pone_assign(world, 0, \"{$var.value}\", va_arg(args, pone_val*));\n";
                }
            }
        }
        $s ~= self!compile(inject-return(.children[2]));
        @*SCOPE.pop();
        $s ~= "pone_pop_scope(world);\n";
        @*TMPS.pop();
        $s ~= "    return pone_nil();\n";
        $s ~= "\}\n";

        @!subs.push: $s;

        if .name {
            qq!pone_assign(world, 0, "&{$name}", pone_mortalize(world, pone_code_new(world, pone_user_func_{$name})))!;
        } else {
            qq!pone_mortalize(world, pone_code_new(world, pone_user_func_{$name}))!;
        }
        */
            break;
        }
        case PVIP_NODE_PARAM: {
            // (param (nop) (variable "$n") (nop) (int 0))
            // (param (nop) (list) (nop) (int 0))
            if (node->children.nodes[1]->type == PVIP_NODE_VARIABLE) {
                PRINTF("pone_assign(world, 0, \"");
                WRITE_PV(node->children.nodes[1]->pv);
                PRINTF("\", va_arg(args, pone_val*));\n");
            }
            break;
        }
        case PVIP_NODE_PARAMS: {
            for (int i=0; i<node->children.size; ++i) {
                COMPILE(node->children.nodes[i]);
            }
            break;
        }
        case PVIP_NODE_USE: {
            // (statements (use (ident "io\/socket\/inet")))
            // TODO use . io/socket/inet;
            // TODO use inet io/socket/inet;
            // TODO use io/socket/inet;
            PRINTF("pone_use(world, \"");
            WRITE_PV(node->children.nodes[1]->pv);
            PRINTF("\", \"");
            WRITE_PV(node->children.nodes[0]->pv);
            PRINTF("\")");
            break;
        }
        case PVIP_NODE_NOP:
            PRINTF("pone_nil()");
            break;
        default:
            fprintf(stderr, "unsupported node '%s'\n", PVIP_node_name(node->type));
            abort();
    }
#undef PRINTF
#undef COMPILE
}

void pone_compile(pone_compile_ctx* ctx, FILE* fp, pone_node* node, int so_no) {
    _pone_compile(ctx, node);

#define PRINTF(fmt, ...) fprintf(fp, (fmt), ##__VA_ARGS__)
    PRINTF("#include \"pone.h\"\n");
    PRINTF("\n");
    PRINTF("// --------------- vvvv ints          vvvv -------------------\n");
    {
        khint_t i;
        for (i=kh_begin(ctx->ints); i!=kh_end(ctx->ints); ++i) {
            if (!kh_exist(ctx->ints, i)) continue;
            pone_int_t key = kh_key(ctx->ints, i);
            PRINTF("static pone_int pone_const_int_%ld = { .type=PONE_INT, .flags=PONE_FLAGS_FROZEN, .i=%ld };\n", key, key);
        }
    }
    PRINTF("// --------------- vvvv functions     vvvv -------------------\n");
    for (int i=0; i<ctx->sub_idx; ++i) {
        fwrite(ctx->subs[i]->buf, 1, ctx->subs[i]->len, fp);
    }
    PRINTF("// --------------- vvvv loader point vvvv -------------------\n");
    PRINTF("void pone_so_init_%d(pone_world* world, pone_val* self, int n, va_list args) {\n", so_no);
    fwrite(ctx->buf->buf, 1, ctx->buf->len, fp);
    PRINTF("}\n");
}

static void pone_compile_node(pone_universe* universe, pone_node* node, const char* filename, bool compile_only, pvip_t* pvip) {
    FILE* fp = fopen("pone_generated.c", "w");
    if (!fp) {
        perror("Cannot open pone_generated.c");
        exit(EXIT_FAILURE);
    }

    pone_compile_ctx ctx;
    memset(&ctx, 0, sizeof(pone_compile_ctx));
    ctx.buf = PVIP_string_new();
    ctx.filename = filename;
    ctx.vars_stack = malloc(sizeof(khash_t(str)*) * 1);
    pone_int_vec_init(&(ctx.loop_stack));
    ctx.ints = kh_init(i64);
    if (!ctx.vars_stack) {
        abort();
    }
    ctx.vars_max = 1;
    ctx.pvip = pvip;
    push_vars_stack(&ctx);
    int so_no = 0;
    pone_compile(&ctx, fp, node, so_no);
    pop_vars_stack(&ctx);
    for (int i=0; i<ctx.sub_idx; ++i) {
        PVIP_string_destroy(ctx.subs[i]);
    }
    if (ctx.subs) {
        free(ctx.subs);
    }
    kh_destroy(i64, ctx.ints);
    free(ctx.vars_stack);
    PVIP_string_destroy(ctx.buf);

    fclose(fp);

    int retval = system("clang -D_POSIX_C_SOURCE=200809L -rdynamic -DPONE_DYNAMIC -fPIC -shared -lstdc++ -Iinclude/ -I src/ -g -lm -std=c99 -o pone_generated.so pone_generated.c -L. -lpone");
    if (retval != 0) {
        fprintf(stderr, "cannot compile code\n");
        exit(EXIT_FAILURE);
    }

    if (!compile_only) {
        void* handle = dlopen("./pone_generated.so", RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "cannot load dll: %s\n", dlerror());
            exit(EXIT_FAILURE);
        }
        pone_funcptr_t pone_so_init = dlsym(handle, "pone_so_init_0");

        char* error;
        if ((error = dlerror()) != NULL)  {
            fprintf(stderr, "%s\n", error);
            exit(EXIT_FAILURE);
        }

        pone_code code = {
          .type = PONE_CODE,
          .flags = PONE_FLAGS_FROZEN,
          .func = pone_so_init,
          .lex = NULL,
        };

        pone_thread_start(universe, (pone_val*)&code);
        pone_universe_wait_threads(universe);

        dlclose(handle);
    }
}

int main(int argc, char** argv) {
    // TODO use portable getopt
    int opt;
    bool dump = false;
    bool compile_only = false;
    const char* eval = NULL;
    bool yy_debug = false;
    while ((opt = getopt(argc, argv, "ycde:")) != -1) {
        switch (opt) {
            case 'c':
                compile_only = true;
                break;
            case 'e':
                eval = optarg;
                break;
            case 'd':
                dump = true;
                break;
            case 'y':
#ifndef YY_DEBUG
                printf("You must recompile pone with -DYY_DEBUG");
                abort();
#endif
                yy_debug = true;
                break;
            default:
                usage();
                exit(EXIT_FAILURE);
        }
    }

    pvip_t* pvip = pvip_new();

    pone_universe* universe = pone_universe_init();
    pone_init(universe);
    if (eval) {
        PVIPString *error;
        pone_node *node = PVIP_parse_string(pvip, eval, strlen(eval), yy_debug, &error);
        if (!node) {
            PVIP_string_say(error);
            PVIP_string_destroy(error);
            printf("ABORT\n");
            exit(1);
        }

        if (dump) {
            PVIP_node_dump_sexp(node);
        } else {
            pone_compile_node(universe, node, "-e", compile_only, pvip);
        }
    } else if (optind < argc) {
        const char* filename = argv[optind];

        FILE* fp = fopen(filename, "r");
        if (!fp) {
            fprintf(stderr, "Cannot open %s\n", filename);
            exit(1);
        }
        PVIPString *error;
        pone_node *node = PVIP_parse_fp(pvip, fp, yy_debug, &error);
        if (!node) {
            PVIP_string_say(error);
            PVIP_string_destroy(error);
            printf("ABORT\n");
            exit(1);
        }
        if (dump) {
            PVIP_node_dump_sexp(node);
        } else {
            pone_compile_node(universe, node, filename, compile_only, pvip);
        }
    } else {
        // REPL mode
        linenoiseSetMultiLine(1);

        char* history_file = strdup(getenv("HOME"));
        if (!history_file) {
            fprintf(stderr, "[pone] cannot allocate memory\n");
            abort();
        }
        history_file = realloc(history_file, strlen(history_file) + strlen("/.pone_history.txt")+1);
        if (!history_file) {
            fprintf(stderr, "[pone] cannot allocate memory\n");
            abort();
        }
        strcpy(history_file+strlen(history_file), "/.pone_history.txt");

        const char* line;
        while((line = linenoise("pone> ")) != NULL) {
            PVIPString *error;
            pone_node *node = PVIP_parse_string(pvip, line, strlen(line), yy_debug, &error);
            if (!node) {
                PVIP_string_say(error);
                PVIP_string_destroy(error);
                printf("ABORT\n");
                exit(1);
            }

            pone_compile_node(universe, node, "-e", false, pvip);

            linenoiseHistoryAdd(line);
            linenoiseHistorySave(history_file);
        }
    }
    pvip_free(pvip);
    pone_universe_destroy(universe);
}

