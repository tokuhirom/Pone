#include "pone.h" /* PONE_INC */

pone_val* pone_regex_new(pone_universe* universe, const char* str, size_t len) {
    pone_val* obj = pone_obj_new(universe, universe->class_regex);
    rockre_regexp* re = rockre_compile(universe->rockre, str, len, true);
    pone_obj_set_ivar_noinc(universe, obj, "$!re", pone_int_new(universe, (long)re));
    pone_obj_set_ivar_noinc(universe, obj, "$!str", pone_str_new_const(universe, str, len));
    return obj;
}

static pone_val* meth_regex_accepts(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);
    assert(sizeof(pone_int_t) == sizeof(void*));

    pone_val* obj = pone_stringify(world, va_arg(args, pone_val*));

    pone_val* re_ptr = pone_obj_get_ivar(world->universe, self, "$!re");
    rockre_region* region = rockre_region_new(world->universe->rockre);
    rockre_regexp* re = (rockre_regexp*)pone_int_val(re_ptr);
    bool b = rockre_partial_match(world->universe->rockre, re, region, pone_str_ptr(obj), pone_str_len(obj));
    if (b) {
        pone_val* match = pone_match_new(world->universe, obj, region->beg[0], region->end[0]);
        for (int i=1; i<region->num_regs; ++i) {
            pone_match_push(world, match, region->beg[i], region->end[i]);
        }
        rockre_region_destroy(world->universe->rockre, region);
        return match;
    } else {
        rockre_region_destroy(world->universe->rockre, region);
        return pone_nil();
    }
}

#define DECLARE_ACCESSOR(name, var) \
    static pone_val* name(pone_world* world, pone_val* self, int n, va_list args) { \
        assert(n == 0); \
        pone_val* obj = va_arg(args, pone_val*); \
        pone_val* v = pone_obj_get_ivar(world->universe, self, var); \
        pone_refcnt_inc(world->universe, v); \
        return v; \
    }

DECLARE_ACCESSOR(meth_regex_str, "$!str")
DECLARE_ACCESSOR(meth_match_from, "$!from")
DECLARE_ACCESSOR(meth_match_to, "$!to")
DECLARE_ACCESSOR(meth_match_orig, "$!orig")
DECLARE_ACCESSOR(meth_match_list, "@!list")

pone_val* pone_match_new(pone_universe* universe, pone_val* orig, int from, int to) {
    assert(universe->class_match);
    pone_val* obj = pone_obj_new(universe, universe->class_match);
    pone_obj_set_ivar_noinc(universe, obj, "$!orig", orig);
    pone_obj_set_ivar_noinc(universe, obj, "$!from", pone_int_new(universe, from));
    pone_obj_set_ivar_noinc(universe, obj, "$!to", pone_int_new(universe, to));
    pone_obj_set_ivar_noinc(universe, obj, "@!list", pone_ary_new(universe, 0));
    return obj;
}

pone_val* pone_match_push(pone_world* world, pone_val* self, int from, int to) {
    pone_val* list = pone_obj_get_ivar(world->universe, self, "@!list");
    pone_val* orig = pone_obj_get_ivar(world->universe, self, "$!orig");
    for (pone_int_t i=0; i<pone_ary_elems(list); ++i) {
        pone_val* c = pone_ary_at_pos(list, i);
        pone_int_t c_from = pone_intify(world, pone_obj_get_ivar(world->universe, c, "$!from"));
        pone_int_t c_to = pone_intify(world, pone_obj_get_ivar(world->universe, c, "$!to"));
        if (from >= c_from) {
            return pone_match_push(world, c, from, to);
        }
    }
    pone_ary_append(world->universe, list, pone_match_new(world->universe, orig, from, to));
}

static pone_val* match_str(pone_world* world,  pone_val* self, int n, int indent) {
    pone_val* orig = pone_obj_get_ivar(world->universe, self, "$!orig");
    pone_int_t from = pone_intify(world, pone_obj_get_ivar(world->universe, self, "$!from"));
    pone_int_t to   = pone_intify(world, pone_obj_get_ivar(world->universe, self, "$!to"));
    pone_val* list = pone_obj_get_ivar(world->universe, self, "@!list");


    pone_val* buf = pone_str_new(world->universe, "", 0);
    for (int i=0; i<indent; ++i) {
        pone_str_append_c(world, buf, " ", strlen(" "));
    }
    if (n>=0) {
        pone_str_appendf(world, buf, "%d ", n);
    }
    pone_str_append_c(world, buf, "｢", strlen("｢"));
    pone_str_append_c(world, buf, pone_str_ptr(orig) + from, to - from);
    pone_str_append_c(world, buf, "｣\n", strlen("｣\n"));
    for (pone_int_t i=0; i<pone_ary_elems(list); ++i) {
        pone_val* match = pone_ary_at_pos(list, i);
        pone_str_append(world, buf, match_str(world, match, i, indent+1));
    }
    return buf;
}

// Match#Str
static pone_val* meth_match_str(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 0);

    return match_str(world, self, -1, 0);
}

void pone_regex_init(pone_universe* universe) {
    assert(universe->class_regex == NULL);

    universe->class_regex = pone_class_new(universe, "Regex", strlen("Regex"));
    // TODO inherit from routine
    pone_class_push_parent(universe, universe->class_regex, universe->class_any);
    pone_add_method_c(universe, universe->class_regex, "ACCEPTS", strlen("ACCEPTS"), meth_regex_accepts);
    pone_add_method_c(universe, universe->class_regex, "Str", strlen("Str"), meth_regex_str);
    pone_class_compose(universe, universe->class_regex);

    // Match class
    universe->class_match = pone_class_new(universe, "Match", strlen("Match"));
    pone_add_method_c(universe, universe->class_match, "from", strlen("from"), meth_match_from);
    pone_add_method_c(universe, universe->class_match, "Str", strlen("Str"), meth_match_str);
    pone_add_method_c(universe, universe->class_match, "to", strlen("to"), meth_match_to);
    pone_add_method_c(universe, universe->class_match, "orig", strlen("orig"), meth_match_orig);
    pone_add_method_c(universe, universe->class_match, "list", strlen("list"), meth_match_list);
    pone_class_push_parent(universe, universe->class_match, universe->class_any);
    pone_class_compose(universe, universe->class_match);
}

