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

void _pone_compile(const char* fname, FILE* fp, PVIPNode* node) {
#define PRINTF(fmt, ...) fprintf(fp, (fmt), ##__VA_ARGS__)
#define WRITE_PV(s) fwrite(s->buf, 1, s->len, fp)
#define LINE(node) PRINTF("#line %d \"%s\"\n", (node)->line_number, fname);
#define COMPILE(node) _pone_compile(fname, fp, node)
#define MORTAL_START PRINTF("pone_mortalize(world,")
#define MORTAL_END PRINTF(")")
    switch (node->type) {
        case PVIP_NODE_STATEMENTS:
            for (int i=0; i<node->children.size; ++i) {
                PVIPNode* child = node->children.nodes[i];
                LINE(child);
                COMPILE(child);
                PRINTF(";\n");
            }
            break;
        case PVIP_NODE_INT:
            MORTAL_START;
            PRINTF("pone_int_new(world->universe, %ld)", node->iv);
            MORTAL_END;
            break;
        case PVIP_NODE_STRING:
            PRINTF("pone_str_new_const(world->universe, \"%s\", %ld)", node->pv->buf, node->pv->len);
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
        case PVIP_NODE_FUNCALL:
            assert(node->children.nodes[0]->type == PVIP_NODE_IDENT);
            PRINTF("pone_builtin_%s(world, ", PVIP_string_c_str(node->children.nodes[0]->pv));
            COMPILE(node->children.nodes[1]);
            PRINTF(")");
            break;
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
        default:
            fprintf(stderr, "unsupported node '%s'\n", PVIP_node_name(node->type));
            abort();
    }
#undef PRINTF
#undef COMPILE
}

void pone_compile(const char* fname, FILE* fp, PVIPNode* node) {
#define PRINTF(fmt, ...) fprintf(fp, (fmt), ##__VA_ARGS__)
    PRINTF("#include \"pone.h\"\n");
    PRINTF("\n");
    PRINTF("// --------------- vvvv functions     vvvv -------------------\n");
    // TODO subs
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
    _pone_compile(fname, fp, node);
    PRINTF("        pone_freetmps(world);\n");
    PRINTF("        pone_pop_scope(world);\n");
    PRINTF("        pone_destroy_world(world);\n");
    PRINTF("        pone_universe_destroy(universe);\n");
    PRINTF("    }\n");
    PRINTF("}\n");
}


static void pone_compiler_eval(const char* src, bool dump, bool compile_only) {
    PVIPString *error;
    PVIPNode *node = PVIP_parse_string(src, strlen(src), false, &error);
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
        pone_compile("-e", fp, node);
        fclose(fp);

        system("clang -I src/ -g -lm -std=c99 -o pone_generated.out pone_generated.c blib/libpone.a");

        if (!compile_only) {
            system("./pone_generated.out");
        }
    }

    PVIP_node_destroy(node);
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
    } else {
        printf("REPL mode is not implemented yet\n");
        abort();
    }
}

