#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <dlfcn.h>
#include "pvip.h"
#include "pone.h"
#include "linenoise.h"

#ifndef CC
#define CC cc
#endif

typedef void (*pone_so_init_t)(pone_world*);

static void usage() {
    printf("Usage: pone -e=code\n"
            "    pone src.pone\n");
}

typedef struct pone_compile_ctx {
    PVIPString* buf;
    PVIPString** subs;
    int sub_idx;

    khash_t(str) **vars_stack;
    int vars_idx;
    int vars_max;

    const char* filename;
    bool want_return;
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

static bool is_builtin(const char* name) {
    return
        strcmp(name, "print")==0
        || strcmp(name, "say")==0
        || strcmp(name, "dd")==0
        || strcmp(name, "abs")==0
        || strcmp(name, "elems")==0
        || strcmp(name, "getenv")==0
        || strcmp(name, "time")==0
        || strcmp(name, "signal")==0
        || strcmp(name, "sleep")==0
        || strcmp(name, "die")==0
        || strcmp(name, "printf")==0
        || strcmp(name, "slurp")==0;
}

void _pone_compile(pone_compile_ctx* ctx, PVIPNode* node) {
#define PRINTF(fmt, ...) PVIP_string_printf(ctx->buf, fmt,  ##__VA_ARGS__)
#define WRITE_PV(pv) PVIP_string_concat(ctx->buf, pv->buf, pv->len)
#define LINE(node) PRINTF("#line %d \"%s\"\n", (node)->line_number, ctx->filename);
#define COMPILE(node) _pone_compile(ctx, node)
#define SAVE_START() PRINTF("pone_save_tmp(world,");
#define SAVE_END() PRINTF(")");
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

    switch (node->type) {
        case PVIP_NODE_STATEMENTS:
            for (int i=0; i<node->children.size; ++i) {
                PVIPNode* child = node->children.nodes[i];
                int n = (child)->line_number;
                const char* filename = ctx->filename;
                PRINTF("#line %d \"%s\"\n", n, filename);
                if (ctx->want_return && i==node->children.size-1 && child->type != PVIP_NODE_FOR && child->type != PVIP_NODE_RETURN && child->type != PVIP_NODE_IF) {
                    PRINTF("return ");
                    COMPILE(child);
                } else {
                    COMPILE(child);
                }
                PRINTF(";\n");
                PRINTF("pone_signal_handle(world);\n");
            }
            break;
        case PVIP_NODE_NUMBER:
            SAVE_START();
            PRINTF("pone_num_new(world, %f)", node->nv);
            SAVE_END();
            break;
        case PVIP_NODE_RANGE:
            SAVE_START();
            PRINTF("pone_range_new(world, ");
            COMPILE(node->children.nodes[0]);
            PRINTF(",");
            COMPILE(node->children.nodes[1]);
            PRINTF(")");
            SAVE_END();
            break;
        case PVIP_NODE_INT:
            SAVE_START();
            PRINTF("pone_int_new(world, %ld)", node->iv);
            SAVE_END();
            break;
        case PVIP_NODE_REGEXP:
            SAVE_START();
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
            SAVE_END();
            break;
        case PVIP_NODE_PAIR: {
            // (pair (ident "a") (int 3))
            PVIPNode* key = node->children.nodes[0];
            PVIPNode* value = node->children.nodes[1];
            SAVE_START();
            PRINTF("pone_pair_new(world, ");
            if (key->type == PVIP_NODE_IDENT) {
                key->type = PVIP_NODE_STRING;
            }
            COMPILE(key);
            PRINTF(",");
            COMPILE(value);
            PRINTF(")");
            SAVE_END();
            break;
        }
        case PVIP_NODE_STRING:
            SAVE_START();
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
            SAVE_END();
            break;
#define INFIX(func) do { SAVE_START(); PRINTF("%s(world, ", func); COMPILE(node->children.nodes[0]);  PRINTF(","); COMPILE(node->children.nodes[1]); PRINTF(")"); SAVE_END(); } while (0)
        case PVIP_NODE_MY:
            // (my (nop) (variable "$inc"))
            def_lex(ctx, PVIP_string_c_str(node->children.nodes[1]->pv));
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
        case PVIP_NODE_STRING_CONCAT:
            INFIX("pone_str_concat");
            break;
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
                    SAVE_START();
                    PRINTF("pone_smart_match(world,");
                    COMPILE(node->children.nodes[0]);
                    PRINTF(",");
                    COMPILE(node->children.nodes[1]->children.nodes[0]);
                    PRINTF(")");
                    SAVE_END();
                    break;
                default:
                    fprintf(stderr, "unsupported chain node '%s'\n", PVIP_node_name(node->children.nodes[1]->type));
                    abort();
            }
            break;
        case PVIP_NODE_ATPOS:
            // (atpos (variable "$a") (int 0))
            SAVE_START();
            PRINTF("pone_at_pos(world, ");
            COMPILE(node->children.nodes[0]);
            PRINTF(",");
            COMPILE(node->children.nodes[1]);
            PRINTF(")");
            SAVE_END();
            break;
        case PVIP_NODE_ATKEY:
            // (atkey (variable "$a") (int 0))
            SAVE_START();
            PRINTF("pone_at_key(world, ");
            COMPILE(node->children.nodes[0]);
            PRINTF(",");
            COMPILE(node->children.nodes[1]);
            PRINTF(")");
            SAVE_END();
            break;
        case PVIP_NODE_IT_METHODCALL:
            // (it_methodcall (ident "say"))
            SAVE_START();
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
            SAVE_END();
            break;
        case PVIP_NODE_META_METHOD_CALL:
            // (meta_method_call (string "3.14") (ident "methods") (nop))
            SAVE_START();
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
            SAVE_END();
            break;
        case PVIP_NODE_METHODCALL:
            // (methodcall (variable "$a") (ident "pop") (args))
            // (atpos (variable "$a") (int 0))
            SAVE_START();
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
            SAVE_END();
            break;
        case PVIP_NODE_FOR: {
            PRINTF("{\n");
            PRINTF("    pone_val* iter = pone_save_tmp(world, pone_iter_init(world, ");
            COMPILE(node->children.nodes[0]);
            PRINTF("));\n");
            PRINTF("    while (true) {\n");
            PRINTF("        pone_val* next = pone_save_tmp(world, pone_iter_next(world, iter));\n");
            PRINTF("        if (next == world->universe->instance_iteration_end) {\n");
            PRINTF("            break;\n");
            PRINTF("        }\n");
            def_lex(ctx, "$_");
            PRINTF("        pone_assign(world, 0, \"$_\", next);\n");
            COMPILE(node->children.nodes[1]);
            PRINTF(";\n");
            PRINTF("    }\n");
            PRINTF("}\n");
            break;
        }
        case PVIP_NODE_WHILE: {
            PRINTF("while (pone_so(");
            COMPILE(node->children.nodes[0]);
            PRINTF(")) {\n");
            COMPILE(node->children.nodes[1]);
            PRINTF("}\n");
            break;
        }
        case PVIP_NODE_UNARY_MINUS: // Negative numeric context operator.
            SAVE_START();
            PRINTF("pone_subtract(world, pone_int_new(world, 0),");
            COMPILE(node->children.nodes[0]);
            PRINTF(")");
            SAVE_END();
            break;
        case PVIP_NODE_UNARY_PLUS: // Negative numeric context operator.
            SAVE_START();
            PRINTF("pone_add(world, pone_int_new(world, 0),");
            COMPILE(node->children.nodes[0]);
            PRINTF(")");
            SAVE_END();
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
            bool builtin = is_builtin(name);
            SAVE_START();
            if (builtin) {
                PRINTF("pone_builtin_%s(world", name);
            } else {
                PRINTF("pone_code_call(world, pone_get_lex(world, \"%s%s\"), pone_nil(), %d",
                        node->children.nodes[0]->type == PVIP_NODE_IDENT ? "&" : "",
                        name, node->children.nodes[1]->children.size);
            }
            if (node->children.nodes[1]->children.size > 0) {
                PRINTF(",");
            }
            COMPILE(node->children.nodes[1]);
            PRINTF(")");
            SAVE_END();
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
            SAVE_START();
            PRINTF("pone_ary_new(world, %d",
                    node->children.size);
            for (int i=0; i<node->children.size; ++i) {
                PRINTF(",");
                COMPILE(node->children.nodes[i]);
            }
            PRINTF(")");
            SAVE_END();
            break;
        case PVIP_NODE_LIST:
            SAVE_START();
            PRINTF("pone_ary_new(world, %d",
                    node->children.size);
            for (int i=0; i<node->children.size; ++i) {
                PRINTF(",");
                COMPILE(node->children.nodes[i]);
            }
            PRINTF(")");
            SAVE_END();
            break;
        case PVIP_NODE_LIST_ASSIGNMENT: {
            PVIPNode* varnode = node->children.nodes[0];
            PVIPString *var;
            SAVE_START();
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
                PVIPNode* var = varnode->children.nodes[0];
                PVIPNode* pos = varnode->children.nodes[1];
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
                PVIPNode* var = varnode->children.nodes[0];
                PVIPNode* key = varnode->children.nodes[1];
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
            SAVE_END();
            break;
        }
        case PVIP_NODE_IDENT: {
            SAVE_START();
            PRINTF("pone_get_lex(world, \"");
            WRITE_PV(node->pv);
            PRINTF("\")");
            SAVE_END();
            break;
        }
        case PVIP_NODE_VARIABLE: {
            SAVE_START();
            PRINTF("pone_get_lex(world, \"");
            WRITE_PV(node->pv);
            PRINTF("\")");
            SAVE_END();
            break;
        }
        case PVIP_NODE_ARGS:
            for (int i=0; i<node->children.size; ++i) {
                PVIPNode* child = node->children.nodes[i];
                COMPILE(child);
                if (i!=node->children.size-1) {
                    PRINTF(",");
                }
            }
            break;
        case PVIP_NODE_HASH:
            // (statements (hash (pair (ident "a") (int 3))))
            SAVE_START();
            PRINTF("pone_hash_assign_keys(world, pone_hash_new(world), %d", node->children.size*2);
            for (int i=0; i<node->children.size; ++i) {
                PRINTF(",");

                PVIPNode* child = node->children.nodes[i];
                if (child->children.nodes[0]->type == PVIP_NODE_IDENT) {
                    WRITE_PV(child->children.nodes[0]->pv);
                } else {
                    COMPILE(child->children.nodes[0]);
                }
                PRINTF(",");
                COMPILE(child->children.nodes[1]);
            }
            PRINTF(")");
            SAVE_END();
            break;
        case PVIP_NODE_RETURN: {
            PRINTF("return ");
            COMPILE(node->children.nodes[0]);
            PRINTF(";\n");
            break;
        }
        case PVIP_NODE_SPECIAL_VARIABLE_EXCEPTIONS: { // $!
            PRINTF("pone_errvar(world)");
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
            bool orig_want_return = ctx->want_return;
            ctx->want_return = true;
            COMPILE(node->children.nodes[0]);
            ctx->want_return = orig_want_return;
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
            PVIPNode* name = node->children.nodes[0];
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
            bool orig_want_return = ctx->want_return;
            ctx->want_return = true;
            COMPILE(node->children.nodes[3]);
            ctx->want_return = orig_want_return;
            POP_SCOPE();
            PRINTF("  return pone_nil();\n");
            PRINTF("}\n");

            RESTORE_BUF;

            SAVE_START();
            if (is_anon) {
                PRINTF("pone_code_new(world, pone_user_func_anon_%d)",
                        ctx->anon_sub_no);
            } else {
                PRINTF("pone_assign(world, 0, \"&%s\", pone_code_new(world, pone_user_func_%s))",
                        PVIP_string_c_str(name->pv),
                        PVIP_string_c_str(name->pv));
            }
            SAVE_END();

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
        case PVIP_NODE_NOP:
            break;
        default:
            fprintf(stderr, "unsupported node '%s'\n", PVIP_node_name(node->type));
            abort();
    }
#undef PRINTF
#undef COMPILE
}

void pone_compile(pone_compile_ctx* ctx, FILE* fp, PVIPNode* node, int so_no) {
    _pone_compile(ctx, node);

#define PRINTF(fmt, ...) fprintf(fp, (fmt), ##__VA_ARGS__)
    PRINTF("#include \"pone.h\"\n");
    PRINTF("\n");
    PRINTF("// --------------- vvvv functions     vvvv -------------------\n");
    for (int i=0; i<ctx->sub_idx; ++i) {
        fwrite(ctx->subs[i]->buf, 1, ctx->subs[i]->len, fp);
    }
    PRINTF("// --------------- vvvv loader point vvvv -------------------\n");
    PRINTF("void pone_so_init_%d(pone_world* world) {\n", so_no);
    fwrite(ctx->buf->buf, 1, ctx->buf->len, fp);
    PRINTF("}\n");
    PRINTF("// --------------- vvvv main function vvvv -------------------\n");
    PRINTF("#ifndef PONE_DYNAMIC\n");
    PRINTF("int main(int argc, const char **argv) {\n");
    PRINTF("    pone_universe* universe = pone_universe_init();\n");
    PRINTF("    pone_world* world = pone_world_new(universe);\n");
    PRINTF("    pone_init(world);\n");
    PRINTF("    world->err_handler_lexs[0] = world->lex;\n");
    PRINTF("    if (setjmp(world->err_handlers[0])) {\n");
    PRINTF("        pone_universe_default_err_handler(world);\n");
    PRINTF("    } else {\n");
    PRINTF("        pone_push_scope(world);\n");
    PRINTF("        pone_so_init_%d(world);\n", so_no);
    PRINTF("        pone_pop_scope(world);\n");
    PRINTF("        pone_world_free(world);\n");
    PRINTF("        pone_universe_destroy(universe);\n");
    PRINTF("    }\n");
    PRINTF("}\n");
    PRINTF("#endif\n");
}

static void pone_compile_node(PVIPNode* node, const char* filename, bool compile_only) {
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
    if (!ctx.vars_stack) {
        abort();
    }
    ctx.vars_max = 1;
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
    free(ctx.vars_stack);
    PVIP_string_destroy(ctx.buf);

    fclose(fp);

    system("clang -rdynamic -DPONE_DYNAMIC -fPIC -shared -lstdc++ -I3rd/rockre/include/ -I src/ -g -lm -std=c99 -o pone_generated.so pone_generated.c -L. -lpone 3rd/rockre/librockre.a");

    if (!compile_only) {
        void* handle = dlopen("./pone_generated.so", RTLD_LAZY);
        if (!handle) {
            fprintf(stderr, "cannot load dll: %s\n", dlerror());
            exit(EXIT_FAILURE);
        }
        pone_so_init_t pone_so_init = (pone_so_init_t)dlsym(handle, "pone_so_init_0");


        char* error;
        if ((error = dlerror()) != NULL)  {
            fprintf(stderr, "%s\n", error);
            exit(EXIT_FAILURE);
        }


        pone_universe* universe = pone_universe_init();
        pone_world* world = pone_world_new(universe);
        pone_init(world);
        world->err_handler_lexs[0] = world->lex;
        if (setjmp(world->err_handlers[0])) {
            pone_universe_default_err_handler(world);
        } else {
            pone_push_scope(world);
            pone_so_init(world);
            pone_pop_scope(world);
            pone_world_free(world);
            pone_universe_destroy(universe);
        }

        dlclose(handle);
    }
}

int main(int argc, char** argv) {
    // TODO use portable getopt
    int opt;
    bool dump = false;
    bool compile_only = false;
    const char* eval = NULL;
    while ((opt = getopt(argc, argv, "cde:")) != -1) {
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
            default:
                usage();
                exit(EXIT_FAILURE);
        }
    }

    pvip_t* pvip = pvip_new();
    if (eval) {
        PVIPString *error;
        PVIPNode *node = PVIP_parse_string(pvip, eval, strlen(eval), false, &error);
        if (!node) {
            PVIP_string_say(error);
            PVIP_string_destroy(error);
            printf("ABORT\n");
            exit(1);
        }

        if (dump) {
            PVIP_node_dump_sexp(node);
        } else {
            pone_compile_node(node, "-e", compile_only);
        }
    } else if (optind < argc) {
        const char* filename = argv[optind];

        FILE* fp = fopen(filename, "r");
        if (!fp) {
            fprintf(stderr, "Cannot open %s\n", filename);
            exit(1);
        }
        PVIPString *error;
        PVIPNode *node = PVIP_parse_fp(pvip, fp, false, &error);
        if (!node) {
            PVIP_string_say(error);
            PVIP_string_destroy(error);
            printf("ABORT\n");
            exit(1);
        }
        if (dump) {
            PVIP_node_dump_sexp(node);
        } else {
            pone_compile_node(node, filename, compile_only);
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
            PVIPNode *node = PVIP_parse_string(pvip, line, strlen(line), false, &error);
            if (!node) {
                PVIP_string_say(error);
                PVIP_string_destroy(error);
                printf("ABORT\n");
                exit(1);
            }

            pone_compile_node(node, "-e", false);

            linenoiseHistoryAdd(line);
            linenoiseHistorySave(history_file);
        }
    }
    pvip_free(pvip);
}

