#define _POSIX_C_SOURCE 199506L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include "pvip.h"

#ifndef CC
#define CC cc
#endif

static void usage() {
    printf("Usage: pone -e=code\n"
            "    pone src.pone\n");
}

typedef struct pone_compile_ctx {
    PVIPString* buf;
    PVIPString** subs;
    int sub_idx;
    const char* filename;
    bool want_return;
    int anon_sub_no;
} pone_compile_ctx;

void _pone_compile(pone_compile_ctx* ctx, PVIPNode* node) {
#define PRINTF(fmt, ...) PVIP_string_printf(ctx->buf, fmt,  ##__VA_ARGS__)
#define WRITE_PV(pv) PVIP_string_concat(ctx->buf, pv->buf, pv->len)
#define LINE(node) PRINTF("#line %d \"%s\"\n", (node)->line_number, ctx->filename);
#define COMPILE(node) _pone_compile(ctx, node)
#define MORTAL_START PRINTF("pone_mortalize(world,")
#define MORTAL_END PRINTF(")")

    switch (node->type) {
        case PVIP_NODE_STATEMENTS:
            for (int i=0; i<node->children.size; ++i) {
                PVIPNode* child = node->children.nodes[i];
                LINE(child);
                if (ctx->want_return && i==node->children.size-1) {
                    PRINTF("return ");
                    COMPILE(child);
                } else {
                    COMPILE(child);
                }
                PRINTF(";\n");
            }
            break;
        case PVIP_NODE_INT:
            MORTAL_START;
            PRINTF("pone_int_new(world->universe, %ld)", node->iv);
            MORTAL_END;
            break;
        case PVIP_NODE_STRING:
            MORTAL_START;
            PRINTF("pone_str_new_const(world->universe, \"");
            WRITE_PV(node->pv);
            PRINTF("\", %ld)", node->pv->len);
            MORTAL_END;
            break;
#define INFIX(func) do { PRINTF("%s(world, ", func); COMPILE(node->children.nodes[0]);  PRINTF(","); COMPILE(node->children.nodes[1]); PRINTF(")"); } while (0)
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
#undef INFIX
        case PVIP_NODE_UNARY_MINUS: // Negative numeric context operator.
            PRINTF("pone_subtract(world, pone_int_new(world->universe, 0),");
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
            if (
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
                    ) {
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
            MORTAL_START;
            PRINTF("pone_ary_new(world->universe, %d",
                    node->children.size);
            for (int i=0; i<node->children.size; ++i) {
                PRINTF(",");
                COMPILE(node->children.nodes[i]);
            }
            PRINTF(")");
            MORTAL_END;
            break;
        case PVIP_NODE_LIST_ASSIGNMENT: {
            PVIPNode* varnode = node->children.nodes[0];
            PVIPString *var;
            switch (varnode->type) {
            case PVIP_NODE_MY:
                var = varnode->children.nodes[1]->pv;
                break;
            case PVIP_NODE_VARIABLE:
                var = varnode->pv;
                break;
            default:
                fprintf(stderr, "invalid node at lhs at line %d\n",
                        node->line_number);
                abort();
            }
            PRINTF("pone_assign(world, 0, \"");
            WRITE_PV(var);
            PRINTF("\", ");
            COMPILE(node->children.nodes[1]);
            PRINTF(")");
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
                PVIPNode* child = node->children.nodes[i];
                COMPILE(child);
                if (i!=node->children.size-1) {
                    PRINTF(",");
                }
            }
            break;
        case PVIP_NODE_HASH:
            // (statements (hash (pair (ident "a") (int 3))))
            MORTAL_START;
            PRINTF("pone_hash_puts(world, pone_hash_new(world->universe), %d", node->children.size*2);
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
            MORTAL_END;
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
            PRINTF("  pone_savetmps(world);\n");
            PRINTF("  pone_push_scope(world);\n");
            if (node->children.size > 0) {
                COMPILE(node->children.nodes[0]);
            }
            PRINTF("  pone_pop_scope(world);\n");
            PRINTF("  pone_freetmps(world);\n");
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
            PRINTF("  pone_savetmps(world);\n");
            PRINTF("  pone_push_scope(world);\n");
            bool orig_want_return = ctx->want_return;
            ctx->want_return = true;
            COMPILE(node->children.nodes[0]);
            ctx->want_return = orig_want_return;
            PRINTF("  pone_pop_scope(world);\n");
            PRINTF("  pone_freetmps(world);\n");
            PRINTF("  return pone_nil();\n");
            PRINTF("}\n");

            RESTORE_BUF;

            PRINTF("pone_mortalize(world, pone_try(world, pone_mortalize(world, pone_code_new(world, pone_try_%d))))",
                    ctx->anon_sub_no);

            break;
        }
        case PVIP_NODE_FUNC: {
            // (func (ident "x") (params) (nop) (block (statements (funcall (ident "say") (args (int 3))))))
            // (func (ident "x") (params (param (nop) (variable "$n") (nop) (int 0))) (nop) (block (statements (funcall (ident "say") (args (variable "$n"))))))
            PVIPNode* name = node->children.nodes[0];
            int argcnt = node->children.nodes[1]->children.size;

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
            PRINTF("  pone_savetmps(world);\n");
            PRINTF("  pone_push_scope(world);\n");
            COMPILE(node->children.nodes[1]);
            bool orig_want_return = ctx->want_return;
            ctx->want_return = true;
            COMPILE(node->children.nodes[3]);
            ctx->want_return = orig_want_return;
            PRINTF("  pone_pop_scope(world);\n");
            PRINTF("  pone_freetmps(world);\n");
            PRINTF("  return pone_nil();\n");
            PRINTF("}\n");

            RESTORE_BUF;

            if (is_anon) {
                PRINTF("pone_mortalize(world, pone_code_new(world, pone_user_func_anon_%d))",
                        ctx->anon_sub_no);
            } else {
                PRINTF("pone_assign(world, 0, \"&%s\", pone_mortalize(world, pone_code_new(world, pone_user_func_%s)))",
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
        $s ~= "pone_savetmps(world);\n";
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
        $s ~= "pone_freetmps(world);\n";
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

void pone_compile(pone_compile_ctx* ctx, FILE* fp, PVIPNode* node) {
    _pone_compile(ctx, node);

#define PRINTF(fmt, ...) fprintf(fp, (fmt), ##__VA_ARGS__)
    PRINTF("#include \"pone.h\"\n");
    PRINTF("\n");
    PRINTF("// --------------- vvvv functions     vvvv -------------------\n");
    for (int i=0; i<ctx->sub_idx; ++i) {
        fwrite(ctx->subs[i]->buf, 1, ctx->subs[i]->len, fp);
    }
    PRINTF("// --------------- vvvv main function vvvv -------------------\n");
    PRINTF("int main(int argc, const char **argv) {\n");
    PRINTF("    pone_init();\n");
    PRINTF("    pone_universe* universe = pone_universe_init();\n");
    PRINTF("    pone_world* world = pone_world_new(universe);\n");
    PRINTF("    universe->err_handler_worlds[0] = world;\n");
    PRINTF("    if (setjmp(universe->err_handlers[0])) {\n");
    PRINTF("        pone_universe_default_err_handler(world);\n");
    PRINTF("    } else {\n");
    PRINTF("        pone_savetmps(world);\n");
    PRINTF("        pone_push_scope(world);\n");
    fwrite(ctx->buf->buf, 1, ctx->buf->len, fp);
    PRINTF("        pone_freetmps(world);\n");
    PRINTF("        pone_pop_scope(world);\n");
    PRINTF("        pone_destroy_world(world);\n");
    PRINTF("        pone_universe_destroy(universe);\n");
    PRINTF("    }\n");
    PRINTF("}\n");
}

static void pone_compiler_eval(const char* src, bool dump, bool compile_only) {
    pvip_t* pvip = pvip_new();
    pone_compile_ctx ctx;
    memset(&ctx, 0, sizeof(pone_compile_ctx));
    ctx.buf = PVIP_string_new();
    ctx.filename = "-e";

    PVIPString *error;
    PVIPNode *node = PVIP_parse_string(pvip, src, strlen(src), false, &error);
    if (!node) {
        PVIP_string_say(error);
        PVIP_string_destroy(error);
        printf("ABORT\n");
        exit(1);
    }

    if (dump) {
        PVIP_node_dump_sexp(node);
    } else {
        FILE* fp = fopen("pone_generated.c", "w");
        if (!fp) {
            perror("Cannot open pone_generated.c");
            exit(EXIT_FAILURE);
        }
        pone_compile(&ctx, fp, node);
        fclose(fp);

        system("clang -I src/ -g -lm -std=c99 -o pone_generated.out pone_generated.c blib/libpone.a");

        if (!compile_only) {
            system("./pone_generated.out");
        }
    }

    for (int i=0; i<ctx.sub_idx; ++i) {
        PVIP_string_destroy(ctx.subs[i]);
    }
    if (ctx.subs) {
        free(ctx.subs);
    }
    PVIP_string_destroy(ctx.buf);
    pvip_free(pvip);
}

static void pone_compile_file(const char* filename, bool dump, bool compile_only) {
    pvip_t* pvip = pvip_new();
    pone_compile_ctx ctx;
    memset(&ctx, 0, sizeof(pone_compile_ctx));
    ctx.buf = PVIP_string_new();
    ctx.filename = filename;

    FILE* fp = fopen(filename, "r");
    if (!fp) {
        fprintf(stderr, "Cannot open %s", filename);
        abort();
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
        FILE* fp = fopen("pone_generated.c", "w");
        if (!fp) {
            perror("Cannot open pone_generated.c");
            exit(EXIT_FAILURE);
        }
        pone_compile(&ctx, fp, node);
        fclose(fp);

        system("clang -I src/ -g -lm -std=c99 -o pone_generated.out pone_generated.c blib/libpone.a");

        if (!compile_only) {
            system("./pone_generated.out");
        }
    }

    for (int i=0; i<ctx.sub_idx; ++i) {
        PVIP_string_destroy(ctx.subs[i]);
    }
    if (ctx.subs) {
        free(ctx.subs);
    }
    PVIP_string_destroy(ctx.buf);
    pvip_free(pvip);
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

    if (eval) {
        pone_compiler_eval(eval, dump, compile_only);
    } else if (optind < argc) {
        const char* fname = argv[optind];
        pone_compile_file(fname, dump, compile_only);
    } else {
        printf("REPL mode is not implemented yet\n");
        abort();
    }
}

