#include "pone.h"
#include <time.h>

// TODO Time#strftime
// TODO we need portable strftime implementation.

#define GET_TM() ((struct tm*)pone_str_ptr(pone_obj_get_ivar(world, self, "$!tm")))

PONE_FUNC(meth_str) {
    PONE_ARG("Time#Str", "");
    struct tm* t = (struct tm*)pone_str_ptr(pone_obj_get_ivar(world, self, "$!tm"));
    char buf[30];
    size_t len = strftime(buf, sizeof(buf), "%Y-%m-%dT%H:%M:%S%z", t);
    return pone_str_new_strdup(world, buf, len);
}

PONE_FUNC(meth_year) {
    PONE_ARG("Time#year", "");
    return pone_int_new(world, GET_TM()->tm_year + 1900);
}

PONE_FUNC(builtin_gmtime) {
    PONE_ARG("gmtime", "");
    time_t     current_time;
    pone_val* t = pone_bytes_new_malloc(world, sizeof(struct tm));

    time(&current_time);
    assert(gmtime_r( &current_time, (struct tm*)pone_str_ptr(t)));
    pone_val* klass = pone_get_lex(world, "$!time_class");
    pone_val* obj = pone_obj_new(world, klass);
    pone_obj_set_ivar(world, obj, "$!tm", t);
    return obj;
}

PONE_FUNC(builtin_localtime) {
    PONE_ARG("localtime", "");
    time_t     current_time;
    pone_val* t = pone_bytes_new_malloc(world, sizeof(struct tm));

    time(&current_time);
    assert(localtime_r( &current_time, (struct tm*)pone_str_ptr(t)));
    pone_val* klass = pone_get_lex(world, "$!time_class");
    pone_val* obj = pone_obj_new(world, klass);
    pone_obj_set_ivar(world, obj, "$!tm", t);
    return obj;
}


void pone_time_init(pone_world* world) {
    pone_val* klass = pone_class_new(world, "Time", strlen("Time"));
    pone_add_method_c(world, klass, "Str", strlen("Str"), meth_str);
    pone_add_method_c(world, klass, "year", strlen("year"), meth_year);
    pone_class_compose(world, klass);

    {
        pone_val* code = pone_code_new(world, builtin_gmtime);
        pone_code_bind(world, code, "$!time_class", klass);
        pone_universe_set_global(world->universe, "gmtime", code);
    }
    {
        pone_val* code = pone_code_new(world, builtin_localtime);
        pone_code_bind(world, code, "$!time_class", klass);
        pone_universe_set_global(world->universe, "localtime", code);
    }
}

