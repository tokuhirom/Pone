#include "pone.h"

#define RE_TRACE(fmt, ...) printf("# [pone] "); printf(fmt, ##__VA_ARGS__); printf("\n");

#define MATCH(s) RE_TRACE("'%s' ~~", s); pone_val* match = pone_re_match(world, prog, s, strlen(s));
#define FROM() pone_intify(world, pone_call_method(world, match, "from", 0))
#define TO() pone_intify(world, pone_call_method(world, match, "to", 0))
#define FROM_IS(n) assert(FROM() == n)
#define TO_IS(n) assert(TO() == n)
#define NON_MATCH() assert(!pone_defined(match))

// https://swtch.com/~rsc/regexp/regexp2.html

#define MAXTHREADS 1000

typedef enum pone_re_opcode {
    PONE_RE_CAPTURE_START,
    PONE_RE_CAPTURE_END,
    PONE_RE_HEAD,
    PONE_RE_TAIL,
    PONE_RE_CHAR,
    PONE_RE_MATCH,
    PONE_RE_JMP,
    PONE_RE_SPLIT
} pone_re_opcode;

typedef struct pone_re_inst {
    pone_re_opcode opcode;
    int c;
    struct pone_re_inst *x;
    struct pone_re_inst *y;
} pone_re_inst;

typedef struct pone_re_thread {
    pone_re_inst* pc;
    const char* sp;
} pone_re_thread;

// -----------------------------------------------------------------------------------------

static pone_val* match(pone_world* world, pone_re_inst* prog, const char* input, int len, int offset) {
    pone_re_thread ready[MAXTHREADS];
    pone_re_thread init = { /* create initial thread */
        .pc = prog,
        .sp = input
    };
    pone_re_inst* pc;
    const char* sp;
    ready[0] = init;
    int nready = 1;
    while (nready--) {
        pc = ready[nready].pc;
        sp = ready[nready].sp;

        for (;;) {
            switch (pc->opcode) {
            case PONE_RE_HEAD:
                RE_TRACE("  pc: %d head", pc-prog);
                if (offset > 0) {
                    goto Dead;
                }
                pc++;
                continue;
            case PONE_RE_TAIL:
                RE_TRACE("  pc: %d tail", pc-prog);
                // RE_TRACE("len:%d!=%d, input:%x, sp:%x", len, sp-input, input, sp);
                if (sp-input != len) {
                    goto Dead;
                }
                pc++;
                continue;
            case PONE_RE_CHAR:
                RE_TRACE("  pc: %d char", pc-prog);
                if (*sp != pc->c) {
                    goto Dead;
                }
                pc++;
                sp++;
                continue;
            case PONE_RE_MATCH:
                RE_TRACE("  pc: %d match", pc-prog);
                return pone_match_new(world->universe, offset, sp-input+offset);
            case PONE_RE_JMP:
                RE_TRACE("  pc: %d jmp", pc-prog);
                pc = pc->x;
                continue;
            case PONE_RE_CAPTURE_START:
                // match 1
                pc++;
                continue;
            case PONE_RE_CAPTURE_END:
                pc++;
                continue;
            case PONE_RE_SPLIT: {
                RE_TRACE("  pc: %d split", pc-prog);
                if (nready >= MAXTHREADS) {
                    fprintf(stderr, "regexp overflow");
                    abort();
                }
                /* queue new thread */
                pone_re_thread thr = {
                    .pc = pc->y,
                    .sp = sp
                };
                ready[nready++] = thr;
                pc = pc->x;  /* continue current thread */
                continue;
            }
            }
            abort();
        }
Dead:;
    }
    return pone_nil();
}

pone_val* pone_re_match(pone_world* world, pone_re_inst* prog, const char* str, size_t len) {
    for (size_t i=0; i<len; ++i) {
        pone_val* v = match(world, prog, str+i, len-i, i);
        if (pone_defined(v)) {
            return v;
        }
    }
    return pone_nil();
}

// /hoge/
static void run_tests1(pone_world* world) {
    pone_re_inst prog[5];
    prog[0].opcode = PONE_RE_CHAR;
    prog[0].c      = 'h';
    prog[1].opcode = PONE_RE_CHAR;
    prog[1].c      = 'o';
    prog[2].opcode = PONE_RE_CHAR;
    prog[2].c      = 'g';
    prog[3].opcode = PONE_RE_CHAR;
    prog[3].c      = 'e';
    prog[4].opcode = PONE_RE_MATCH;

    {
        MATCH("hoge");
        FROM_IS(0);
        TO_IS(4);
    }

    {
        MATCH("aaahoge");
        FROM_IS(3);
        TO_IS(7);
    }

    {
        MATCH("uwaa");
        NON_MATCH();
    }
}

// /^hoge/
static void run_tests2(pone_world* world) {
    int n = 0;
    pone_re_inst prog[6];
    prog[n++].opcode = PONE_RE_HEAD;
    prog[n].opcode   = PONE_RE_CHAR;
    prog[n++].c      = 'h';
    prog[n].opcode   = PONE_RE_CHAR;
    prog[n++].c      = 'o';
    prog[n].opcode   = PONE_RE_CHAR;
    prog[n++].c      = 'g';
    prog[n].opcode   = PONE_RE_CHAR;
    prog[n++].c      = 'e';
    prog[n++].opcode = PONE_RE_MATCH;

    {
        MATCH("hoge");
        FROM_IS(0);
        TO_IS(4);
    }
    {
        MATCH("hogeeee");
        FROM_IS(0);
        TO_IS(4);
    }
    {
        MATCH("hige");
        NON_MATCH();
    }
    {
        MATCH("aaahoge");
        NON_MATCH();
    }
}

// -----------------------------------------------------------------------------------------

// /^hoge$/

static void run_tests3(pone_world* world) {
    int n = 0;
    pone_re_inst prog[6];
    prog[n++].opcode = PONE_RE_HEAD;
    prog[n].opcode   = PONE_RE_CHAR;
    prog[n++].c      = 'h';
    prog[n].opcode   = PONE_RE_CHAR;
    prog[n++].c      = 'o';
    prog[n].opcode   = PONE_RE_CHAR;
    prog[n++].c      = 'g';
    prog[n].opcode   = PONE_RE_CHAR;
    prog[n++].c      = 'e';
    prog[n++].opcode = PONE_RE_TAIL;
    prog[n++].opcode = PONE_RE_MATCH;

    {
        MATCH("hoge");
        FROM_IS(0);
        TO_IS(4);
    }
    {
        MATCH("hogeee");
        NON_MATCH();
    }
}

// /e$/
static void run_tests4(pone_world* world) {
    int n = 0;
    pone_re_inst prog[3];
    prog[n  ].c      = 'e';
    prog[n++].opcode = PONE_RE_CHAR;
    prog[n++].opcode = PONE_RE_TAIL;
    prog[n++].opcode = PONE_RE_MATCH;

    {
        MATCH("hoge");
        FROM_IS(3);
        TO_IS(4);
    }
    {
        MATCH("hogeea");
        NON_MATCH();
    }
}

// /^ hog e* $/
static void run_tests5(pone_world* world) {
    // L1: split L2, L3
    // L2: codes for e
    //     jmp L1
    //     L3:

    int n = 0;
    pone_re_inst prog[8];
    prog[n  ].c      = 'h';
    prog[n++].opcode = PONE_RE_CHAR;
    prog[n  ].c      = 'o';
    prog[n++].opcode = PONE_RE_CHAR;
    prog[n  ].c      = 'g';
    prog[n++].opcode = PONE_RE_CHAR;

    pone_re_inst *l1 = &(prog[n]);
    prog[n++].opcode = PONE_RE_SPLIT;

    // L2:
    pone_re_inst *l2 = &(prog[n]);
    prog[n  ].c      = 'e';
    prog[n++].opcode = PONE_RE_CHAR;
    l1->x = l2;

    prog[n  ].x      = l1;
    prog[n++].opcode = PONE_RE_JMP;

    pone_re_inst *l3 = &(prog[n]);
    l1->y = l3;
    prog[n++].opcode = PONE_RE_TAIL;

    prog[n++].opcode = PONE_RE_MATCH;

    {
        MATCH("hog");
        FROM_IS(0);
        TO_IS(3);
    }
    {
        MATCH("hoge");
        FROM_IS(0);
        TO_IS(4);
    }
    {
        MATCH("hogeee");
        FROM_IS(0);
        TO_IS(6);
    }
    {
        MATCH("hogeem");
        NON_MATCH();
    }
}

// /^ hog e* m $/
static void run_tests6(pone_world* world) {
    // L1: split L2, L3
    // L2: codes for e
    //     jmp L1
    //     L3:

    int n = 0;
    pone_re_inst prog[9];
    prog[n  ].c      = 'h';
    prog[n++].opcode = PONE_RE_CHAR;
    prog[n  ].c      = 'o';
    prog[n++].opcode = PONE_RE_CHAR;
    prog[n  ].c      = 'g';
    prog[n++].opcode = PONE_RE_CHAR;

    pone_re_inst *l1 = &(prog[n]);
    prog[n++].opcode = PONE_RE_SPLIT;

    // L2:
    pone_re_inst *l2 = &(prog[n]);
    prog[n  ].c      = 'e';
    prog[n++].opcode = PONE_RE_CHAR;
    l1->x = l2;

    prog[n  ].x      = l1;
    prog[n++].opcode = PONE_RE_JMP;

    pone_re_inst *l3 = &(prog[n]);
    l1->y = l3;

    prog[n  ].c      = 'm';
    prog[n++].opcode = PONE_RE_CHAR;

    prog[n++].opcode = PONE_RE_TAIL;

    prog[n++].opcode = PONE_RE_MATCH;

    {
        MATCH("hogm");
        FROM_IS(0);
        TO_IS(4);
    }
    {
        MATCH("hogem");
        FROM_IS(0);
        TO_IS(5);
    }
    {
        MATCH("hogeeem");
        FROM_IS(0);
        TO_IS(7);
    }
    {
        MATCH("hogeea");
        NON_MATCH();
    }
}

// /^(e)$/
static void run_tests7(pone_world* world) {
    int n=0;
    pone_re_inst prog[6];
    prog[n++].opcode = PONE_RE_HEAD;
    prog[n++].opcode = PONE_RE_CAPTURE_START;
    prog[n  ].c      = 'e';
    prog[n++].opcode = PONE_RE_CHAR;
    prog[n++].opcode = PONE_RE_CAPTURE_END;
    prog[n++].opcode = PONE_RE_TAIL;
    prog[n++].opcode = PONE_RE_MATCH;

    {
        MATCH("e");
        FROM_IS(0);
        TO_IS(1);
    }

    {
        MATCH("uwaa");
        NON_MATCH();
    }
}

// -----------------------------------------------------------------------------------------

int main(int argc, char** argv) {
    pone_universe* universe = pone_universe_init();
    pone_world* world = pone_world_new(universe);

    if (setjmp(universe->err_handlers[0])) {
        world->universe = universe; // magical trash
        pone_universe_default_err_handler(world);
    } else {
        pone_savetmps(world);
        pone_push_scope(world);

        RE_TRACE("/hoge/");
        run_tests1(world);
        RE_TRACE("/^hoge/");
        run_tests2(world);
        RE_TRACE("/^hoge$/");
        run_tests3(world);
        RE_TRACE("/e$/");
        run_tests4(world);
        RE_TRACE("/^hog e* $/");
        run_tests5(world);
        RE_TRACE("/^hog e* m $/");
        run_tests6(world);
        RE_TRACE("/^ (e) $/");
        run_tests7(world);

        pone_freetmps(world);
        pone_pop_scope(world);

        pone_destroy_world(world);
        pone_universe_destroy(universe);
    }
}

