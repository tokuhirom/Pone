#include "pone.h" /* PONE_INC */

pone_val* pone_regex_new(pone_universe* universe, pone_funcptr_t func) {
    pone_val* code = pone_code_new_c(universe, func);
    pone_val* obj = pone_obj_new(universe, universe->class_regex);
    pone_obj_set_ivar_noinc(universe, obj, "$!code", code);
    return obj;
}

static pone_val* meth_regex_accepts(pone_world* world, pone_val* self, int n, va_list args) {
    assert(n == 1);

    pone_val* obj = va_arg(args, pone_val*);

    pone_val* code = pone_obj_get_ivar(world->universe, self, "$!code");
    return pone_code_call(world, code, self, 1, obj);
}

#define DECLARE_ACCESSOR(name, var) \
    static pone_val* name(pone_world* world, pone_val* self, int n, va_list args) { \
        assert(n == 0); \
        pone_val* obj = va_arg(args, pone_val*); \
        return pone_obj_get_ivar(world->universe, self, var); \
    }

DECLARE_ACCESSOR(meth_match_from, "$!from")
DECLARE_ACCESSOR(meth_match_to, "$!to")

pone_val* pone_match_new(pone_universe* universe, int from, int to) {
    pone_val* obj = pone_obj_new(universe, universe->class_match);
    pone_obj_set_ivar_noinc(universe, obj, "$!from", pone_int_new(universe, from));
    pone_obj_set_ivar_noinc(universe, obj, "$!to", pone_int_new(universe, to));
    return obj;
}

void pone_regex_init(pone_universe* universe) {
    assert(universe->class_regex == NULL);

    universe->class_regex = pone_class_new(universe, "Regex", strlen("Regex"));
    // TODO inherit from routine
    pone_class_push_parent(universe, universe->class_regex, universe->class_any);
    pone_add_method_c(universe, universe->class_regex, "ACCEPTS", strlen("ACCEPTS"), meth_regex_accepts);
    pone_class_compose(universe, universe->class_regex);

    universe->class_match = pone_class_new(universe, "Match", strlen("Match"));
    pone_add_method_c(universe, universe->class_match, "from", strlen("from"), meth_match_from);
    pone_add_method_c(universe, universe->class_match, "to", strlen("to"), meth_match_to);
    pone_class_push_parent(universe, universe->class_match, universe->class_any);
    pone_class_compose(universe, universe->class_match);
}

