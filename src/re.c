#include "pone.h" /* PONE_INC */
#include "rockre.h"

pone_val* pone_regex_new(pone_world* world, const char* str, size_t len) {
    pone_universe* universe = world->universe;
    pone_val* obj = pone_obj_new(world, universe->class_regex);
    rockre_regexp* re = rockre_compile(universe->rockre, str, len, true);
    pone_obj_set_ivar(world, obj, "$!re", pone_int_new(world, (long)re));
    pone_obj_set_ivar(world, obj, "$!str", pone_str_new_const(world, str, len));
    return obj;
}

static pone_val* meth_regex_accepts(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);
    assert(sizeof(pone_int_t) == sizeof(void*));

    pone_val* obj = pone_stringify(world, va_arg(args, pone_val*));

    pone_val* re_ptr = pone_obj_get_ivar(world, self, "$!re");
    rockre_region* region = rockre_region_new(world->universe->rockre);
    rockre_regexp* re = (rockre_regexp*)pone_int_val(re_ptr);
    bool b = rockre_partial_match(world->universe->rockre, re, region, pone_str_ptr(obj), pone_str_len(obj));
    if (b) {
        pone_val* match = pone_match_new(world, obj, region->beg[0], region->end[0]);
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

PONE_DECLARE_GETTER(meth_regex_str, "$!str")
PONE_DECLARE_GETTER(meth_match_from, "$!from")
PONE_DECLARE_GETTER(meth_match_to, "$!to")
PONE_DECLARE_GETTER(meth_match_orig, "$!orig")
PONE_DECLARE_GETTER(meth_match_list, "@!list")

pone_val* pone_match_new(pone_world* world, pone_val* orig, pone_int_t from, pone_int_t to) {
    pone_universe* universe = world->universe;
    assert(universe->class_match);
    pone_val* obj = pone_obj_new(world, universe->class_match);
    pone_obj_set_ivar(world, obj, "$!orig", orig);
    pone_obj_set_ivar(world, obj, "$!from", pone_int_new(world, from));
    pone_obj_set_ivar(world, obj, "$!to", pone_int_new(world, to));
    pone_obj_set_ivar(world, obj, "@!list", pone_ary_new(world, 0));
    return obj;
}

void pone_match_push(pone_world* world, pone_val* self, pone_int_t from, pone_int_t to) {
    pone_val* list = pone_obj_get_ivar(world, self, "@!list");
    pone_val* orig = pone_obj_get_ivar(world, self, "$!orig");
    for (pone_int_t i=0; i<pone_ary_elems(list); ++i) {
        pone_val* c = pone_ary_at_pos(list, i);
        pone_int_t c_from = pone_intify(world, pone_obj_get_ivar(world, c, "$!from"));
        if (from >= c_from) {
            return pone_match_push(world, c, from, to);
        }
    }
    pone_ary_append(world->universe, list, pone_match_new(world, orig, from, to));
}

static pone_val* match_str(pone_world* world,  pone_val* self, int n, int indent) {
    pone_val* orig = pone_obj_get_ivar(world, self, "$!orig");
    pone_int_t from = pone_intify(world, pone_obj_get_ivar(world, self, "$!from"));
    pone_int_t to   = pone_intify(world, pone_obj_get_ivar(world, self, "$!to"));
    pone_val* list = pone_obj_get_ivar(world, self, "@!list");


    pone_val* buf = pone_str_new(world, "", 0);
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

void pone_regex_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_regex == NULL);

    universe->class_regex = pone_class_new(world, "Regex", strlen("Regex"));
    // TODO inherit from routine
    pone_class_push_parent(world, universe->class_regex, universe->class_any);
    pone_add_method_c(world, universe->class_regex, "ACCEPTS", strlen("ACCEPTS"), meth_regex_accepts);
    pone_add_method_c(world, universe->class_regex, "Str", strlen("Str"), meth_regex_str);
    pone_class_compose(world, universe->class_regex);

    // Match class
    universe->class_match = pone_class_new(world, "Match", strlen("Match"));
    pone_add_method_c(world, universe->class_match, "from", strlen("from"), meth_match_from);
    pone_add_method_c(world, universe->class_match, "Str", strlen("Str"), meth_match_str);
    pone_add_method_c(world, universe->class_match, "to", strlen("to"), meth_match_to);
    pone_add_method_c(world, universe->class_match, "orig", strlen("orig"), meth_match_orig);
    pone_add_method_c(world, universe->class_match, "list", strlen("list"), meth_match_list);
    pone_class_push_parent(world, universe->class_match, universe->class_any);
    pone_class_compose(world, universe->class_match);

    pone_universe_set_global(universe, "Regex", universe->class_regex);
}

