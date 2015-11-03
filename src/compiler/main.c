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
#define COMPILE(node) _pone_compile(fname, fp, node)
    switch (node->type) {
        case PVIP_NODE_STATEMENTS:
            for (int i=0; i<node->children.size; ++i) {
                PVIPNode* child = node->children.nodes[i];
                PRINTF("#line %d \"%s\"\n", node->line_number, fname);
                COMPILE(child);
                PRINTF(";\n");
            }
            break;
        case PVIP_NODE_INT:
            PRINTF("pone_int_new(world->universe, %ld)", node->iv);
            break;
        case PVIP_NODE_FUNCALL:
            assert(node->children.nodes[0]->type == PVIP_NODE_IDENT);
            PRINTF("pone_builtin_%s(world, ", PVIP_string_c_str(node->children.nodes[0]->pv));
            COMPILE(node->children.nodes[1]);
            PRINTF(")");
            break;
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
            fprintf(stderr, "unsupported node\n");
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

