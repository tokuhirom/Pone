#include "pone.h"
#include "pone_exc.h"
#include <time.h>

// TODO Time#strftime
// TODO we need portable strftime implementation.
// TODO Time#http
// TODO Time#rfc2822

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

PONE_FUNC(meth_month) {
    PONE_ARG("Time#month", "");
    return pone_int_new(world, GET_TM()->tm_mon + 1);
}

PONE_FUNC(meth_mday) {
    PONE_ARG("Time#mday", "");
    return pone_int_new(world, GET_TM()->tm_mday);
}

PONE_FUNC(meth_wday) {
    PONE_ARG("Time#wday", "");
    return pone_int_new(world, GET_TM()->tm_wday);
}

PONE_FUNC(meth_hour) {
    PONE_ARG("Time#hour", "");
    return pone_int_new(world, GET_TM()->tm_hour);
}

PONE_FUNC(meth_min) {
    PONE_ARG("Time#min", "");
    return pone_int_new(world, GET_TM()->tm_min);
}

PONE_FUNC(meth_sec) {
    PONE_ARG("Time#sec", "");
    return pone_int_new(world, GET_TM()->tm_sec);
}

PONE_FUNC(builtin_gmtime) {
    PONE_ARG("gmtime", "");
    time_t current_time;
    pone_val* t = pone_bytes_new_malloc(world, sizeof(struct tm));

    time(&current_time);
    struct tm* tm = gmtime_r(&current_time, (struct tm*)pone_str_ptr(t));
    if (tm == NULL) {
        pone_throw_str(world, "gmtime_r: %s", strerror(errno));
    }
    pone_val* klass = pone_get_lex(world, "$!time_class");
    pone_val* obj = pone_obj_new(world, klass);
    pone_obj_set_ivar(world, obj, "$!tm", t);
    return obj;
}

pone_val* pone_localtime_from_epoch(pone_world* world, time_t epoch) {
    pone_val* t = pone_bytes_new_malloc(world, sizeof(struct tm));
    struct tm* tm = localtime_r(&epoch, (struct tm*)pone_str_ptr(t));
    if (tm == NULL) {
        pone_throw_str(world, "localtime_r: %s", strerror(errno));
    }
    pone_val* obj = pone_obj_new(world, world->universe->class_time);
    pone_obj_set_ivar(world, obj, "$!tm", t);
    return obj;
}

PONE_FUNC(builtin_localtime) {
    PONE_ARG("localtime", "");
    time_t epoch = time(NULL);
    return pone_localtime_from_epoch(world, epoch);
}

void pone_time_init(pone_world* world) {
    pone_val* klass = pone_class_new(world, "Time", strlen("Time"));
    pone_add_method_c(world, klass, "Str", strlen("Str"), meth_str);
    pone_add_method_c(world, klass, "year", strlen("year"), meth_year);
    pone_add_method_c(world, klass, "month", strlen("month"), meth_month);
    pone_add_method_c(world, klass, "mday", strlen("mday"), meth_mday);
    pone_add_method_c(world, klass, "wday", strlen("wday"), meth_wday);
    pone_add_method_c(world, klass, "hour", strlen("hour"), meth_hour);
    pone_add_method_c(world, klass, "min", strlen("min"), meth_min);
    pone_add_method_c(world, klass, "sec", strlen("sec"), meth_sec);
    world->universe->class_time = klass;
    pone_universe_set_global(world->universe, "Time", klass);

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
