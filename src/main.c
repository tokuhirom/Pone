#include "pone.h"
#include "pone_compile.h"
#include "pone_tmpfile.h"
#include "pvip.h"
#include "linenoise.h"

#include <unistd.h> /* getopt */

static void usage() {
    printf("Usage:\n"
           "    pone -e=code\n"
           "    pone path/to/src.pn\n");
}

int main(int argc, char** argv) {
    // initialize universe.
    pone_universe* universe = pone_universe_init();
    pone_init(universe);

    // create compiler world.
    pone_world* world = pone_world_new(universe);

    // parse options.
    // TODO use portable getopt
    int opt;
    bool dump = false;
    bool compile_only = false;
    const char* eval = NULL;
    bool yy_debug = false;
    while ((opt = getopt(argc, argv, "ycde:I:")) != -1) {
        switch (opt) {
        case 'I':
            pone_ary_push(
                    world->universe,
                    universe->inc,
                    pone_str_new_strdup(world, optarg, strlen(optarg)));
            break;
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

    if (setjmp(world->err_handlers[0])) {
        pone_universe_default_err_handler(world);
    } else {
        if (eval) {
            if (dump) {
                pvip_t* pvip = pvip_new();
                pone_node* node = pone_parse_string(world, pvip, eval, yy_debug);
                pone_node_dump_sexp(node);
                pvip_free(pvip);
            } else {
                pone_val* code = pone_compile_str(world, eval);

                if (!compile_only) {
                    pone_thread_start(world->universe, code);
                    pone_universe_wait_threads(world->universe);
                }
            }
        } else if (optind < argc) {
            const char* filename = argv[optind];

            FILE* fp = fopen(filename, "r");
            if (!fp) {
                fprintf(stderr, "Cannot open %s\n", filename);
                exit(1);
            }
            if (dump) {
                pvip_t* pvip = pvip_new();
                pone_node* node = pone_parse_fp(world, pvip, fp, yy_debug);
                pone_node_dump_sexp(node);
                pvip_free(pvip);
            } else {
                pone_val* code = pone_compile_fp(world, fp, filename);
                if (!compile_only) {
                    pone_thread_start(world->universe, code);
                    pone_universe_wait_threads(world->universe);
                }
            }
        } else {
            // REPL mode
            linenoiseSetMultiLine(1);

            char* history_file = strdup(getenv("HOME"));
            if (!history_file) {
                fprintf(stderr, "[pone] cannot allocate memory\n");
                abort();
            }
            history_file = realloc(history_file, strlen(history_file) + strlen("/.pone_history.txt") + 1);
            if (!history_file) {
                fprintf(stderr, "[pone] cannot allocate memory\n");
                abort();
            }
            strcpy(history_file + strlen(history_file), "/.pone_history.txt");

            const char* line;
            while ((line = linenoise("pone> ")) != NULL) {
                pone_push_scope(world);

                pone_val* code = pone_compile_str(world, line);

                pone_thread_start(world->universe, code);
                pone_universe_wait_threads(world->universe);

                linenoiseHistoryAdd(line);
                linenoiseHistorySave(history_file);

                pone_pop_scope(world);

                if (world->gc_requested) {
                    pone_gc_run(world);
                }
            }
        }
    }
    pone_world_free(world);
    pone_universe_destroy(universe);
}
