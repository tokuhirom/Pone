#include "pone.h"

#define RE_TRACE(fmt, ...) printf("# [pone] "); printf(fmt, ##__VA_ARGS__); printf("\n");

#define RE(code) pone_val* re = pone_regex_new(world->universe, code);
#define MATCH(s) RE_TRACE("'%s' ~~", s); pone_val* match = pone_call_method(world, re, "ACCEPTS", 1, pone_str_new_const(world->universe, s, strlen(s)));
#define FROM() pone_intify(world, pone_call_method(world, match, "from", 0))
#define TO() pone_intify(world, pone_call_method(world, match, "to", 0))
#define FROM_IS(n) assert(FROM() == n)
#define TO_IS(n) assert(TO() == n)
#define NON_MATCH() assert(!pone_defined(match))

// -----------------------------------------------------------------------------------------

// /hoge/
static pone_val* pone_regex_1(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);
    int from = 0;

    pone_val* str = va_arg(args, pone_val*);
    const char* orig = pone_str_ptr(str);
    const char* p;
    int l = pone_str_len(str);

START:
    p = orig + from;
    if (memcmp(p, "hoge", 4) == 0) {
        int to = from + 4;
        return pone_match_new(world->universe, from, to);
    }

    from++;
    if (from == l) {
        RE_TRACE("reached to end");
        return pone_nil();
    } else {
        RE_TRACE("next. %d", from);
        goto START;
    }
}

static void run_tests1(pone_world* world) {
    RE(pone_regex_1);
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

// -----------------------------------------------------------------------------------------

// /^hoge/
static pone_val* pone_regex_2(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);
    int from = 0;

    pone_val* str = va_arg(args, pone_val*);
    const char* p = pone_str_ptr(str);
    int l = pone_str_len(str);

START:
    // ^
    if (from != 0) {
        return pone_nil();
    }

    // hoge
    if (memcmp(p+from, "hoge", 4) == 0) {
        int to = from + 4;
        return pone_match_new(world->universe, from, to);
    }

    from++;
    if (from == l) {
        RE_TRACE("reached to end");
        return pone_nil();
    } else {
        RE_TRACE("next. %d", from);
        goto START;
    }
}

// /^hoge/
static void run_tests2(pone_world* world) {
    RE(pone_regex_2);

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
static pone_val* pone_regex_3(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);
    int from = 0;
    int to = 0;

    pone_val* str = va_arg(args, pone_val*);
    const char* orig = pone_str_ptr(str);
    const char* p;
    int l = pone_str_len(str);

START:
    to = from;
    p = orig + from;

    // ^
    if (from != 0) {
        return pone_nil();
    }

    // hoge
    if (memcmp(p, "hoge", 4) == 0) {
        to += 4;
    } else {
        goto NEXT;
    }

    // $
    RE_TRACE("'$' %d != %d", to, l);
    if (to != l) {
        return pone_nil();
    }

    // succeeded
    return pone_match_new(world->universe, from, to);

NEXT:
    from++;
    if (from == l) {
        RE_TRACE("reached to end");
        return pone_nil();
    } else {
        RE_TRACE("next. %d", from);
        goto START;
    }
}

// /^hoge$/
static void run_tests3(pone_world* world) {
    RE(pone_regex_3);

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

// -----------------------------------------------------------------------------------------

// /^ hog e* $/
static pone_val* pone_regex_4(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);
    int from = 0;
    int to = 0;

    pone_val* str = va_arg(args, pone_val*);
    const char* orig_p = pone_str_ptr(str);
    const char* p;
    int l = pone_str_len(str);

START:
    to = from;
    p = orig_p + from;

    // ^
    if (from != 0) {
        return pone_nil();
    }

    // hog
    if (memcmp(p, "hog", 3) == 0) {
        p += 3;
        to += 3;
    } else {
        RE_TRACE("/hog/ match failed");
        goto NEXT;
    }

    // e*
    while (to!=l) {
        RE_TRACE("matching /e*/ %d != %d", to, l);
        if (memcmp(p, "e", 1) == 0) {
            p++;
            to++;
        } else {
            RE_TRACE("%c != 'e'", *p);
            break;
        }
    }

    // $
    RE_TRACE("'$' %d != %d", to, l);
    if (to != l) {
        return pone_nil();
    }

    // succeeded
    return pone_match_new(world->universe, from, to);

NEXT:
    from++;
    if (from == l) {
        RE_TRACE("reached to end");
        return pone_nil();
    } else {
        RE_TRACE("next. %d", from);
        goto START;
    }
}

// /^hoge$/
static void run_tests4(pone_world* world) {
    RE(pone_regex_4);

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
        RE_TRACE("/^hog e* $/");
        run_tests4(world);

        pone_freetmps(world);
        pone_pop_scope(world);

        pone_destroy_world(world);
        pone_universe_destroy(universe);
    }
}

