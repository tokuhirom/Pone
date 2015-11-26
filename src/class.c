#include "pone.h" /* PONE_INC */
#include "pone_opaque.h"
#include "pone_exc.h"

/**
 * Class related operations
 */

PONE_FUNC(meth_name) {
    return pone_obj_get_ivar(world, self, "$!name");
}

PONE_FUNC(meth_Str) {
    pone_val* v = pone_str_new_strdup(world, "Class<", strlen("Class<"));
    pone_str_append(world, v, pone_obj_get_ivar(world, self, "$!name"));
    pone_str_append_c(world, v, ">", strlen(">"));
    return v;
}

PONE_FUNC(mu_str) {
    PONE_ARG("#Str", "");

    pone_val* v = pone_str_new_strdup(world, "", 0);
    pone_str_append(world, v, pone_obj_get_ivar(world, pone_what(world, self), "$!name"));
    pone_str_append_c(world, v, "<", strlen("<"));
    pone_str_append(world, v, pone_str_new_printf(world, "%p", self));
    pone_str_append_c(world, v, ">", strlen(">"));
    return v;
}

PONE_FUNC(mu_say) {
    PONE_ARG("#say", "");

    pone_val* s = pone_str_copy(world, pone_stringify(world, self));
    pone_str_append_c(world, s, "\n", 1);
    fwrite(pone_str_ptr(s), sizeof(char), pone_str_len(s), stdout);
    return pone_nil();
}

PONE_FUNC(meth_accepts) {
    pone_val* val;
    PONE_ARG("Class#ACCEPTS", "o", &val);

    return pone_what(world, val) == self ? pone_true() : pone_false();
}

// initialize Class class
pone_val* pone_init_class(pone_world* world) {
    pone_val* val = pone_obj_alloc(world, PONE_OBJ);
    val->as.obj.ivar = kh_init(str);
    val->as.obj.klass = pone_nil();

    pone_obj_set_ivar(world, val, "$!name", pone_str_new_const(world, "Class", strlen("Class")));
    pone_obj_set_ivar(world, val, "$!methods", pone_hash_new(world));
    pone_add_method_c(world, val, "name", strlen("name"), meth_name);
    pone_add_method_c(world, val, "Str", strlen("Str"), meth_Str);
    pone_add_method_c(world, val, "say", strlen("say"), mu_say);
    pone_add_method_c(world, val, "ACCEPTS", strlen("ACCEPTS"), meth_accepts);

    return val;
}

pone_val* pone_what(pone_world* world, pone_val* obj) {
    assert(pone_alive(obj));
    pone_universe* universe = world->universe;

    switch (pone_type(obj)) {
    case PONE_NIL:
        return universe->class_nil;
    case PONE_INT:
        return universe->class_int;
    case PONE_NUM:
        return universe->class_num;
    case PONE_STRING:
        if (obj->as.basic.flags & PONE_FLAGS_STR_UTF8) {
            return universe->class_str;
        } else {
            return universe->class_bytes;
        }
    case PONE_ARRAY:
        return universe->class_ary;
    case PONE_BOOL:
        return universe->class_bool;
    case PONE_HASH:
        return universe->class_hash;
    case PONE_CODE:
        return universe->class_code;
    case PONE_OPAQUE:
        assert(obj->as.opaque.body->klass);
        return obj->as.opaque.body->klass;
    case PONE_OBJ:
        return obj->as.obj.klass;
    case PONE_LEX:
        abort();
    }
    abort();
}

/**
 * Create new instance of class
 */
pone_val* pone_class_new(pone_world* world, const char* name, size_t name_len) {
    pone_val* obj = pone_obj_new(world, world->universe->class_class);
    pone_obj_set_ivar(world, obj, "$!name", pone_str_new_strdup(world, name, name_len));
    pone_obj_set_ivar(world, obj, "$!methods", pone_hash_new(world));

    pone_add_method_c(world, obj, "say", strlen("say"), mu_say);
    pone_add_method_c(world, obj, "Str", strlen("Str"), mu_str);

    return (pone_val*)obj;
}

void pone_add_method_c(pone_world* world, pone_val* klass, const char* name, size_t name_len, pone_funcptr_t funcptr) {
    assert(klass);
    pone_val* code = pone_code_new_c(world, funcptr);
    pone_add_method(world, klass, name, name_len, code);
}

void pone_add_method(pone_world* world, pone_val* klass, const char* name, size_t name_len, pone_val* method) {
    assert(klass);
    assert(pone_type(klass) == PONE_OBJ);
    assert(pone_type(method) == PONE_CODE);

    pone_val* methods = pone_obj_get_ivar(world, klass, "$!methods");
    assert(pone_type(methods) == PONE_HASH);
    pone_hash_assign_key_c(world, methods, name, name_len, method);
}

const char* pone_what_str_c(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_NIL:
        return "Nil";
    case PONE_INT:
        return "Int";
    case PONE_NUM:
        return "Num";
    case PONE_STRING:
        if (val->as.basic.flags & PONE_FLAGS_STR_UTF8) {
            return "Str";
        } else {
            return "Bytes";
        }
    case PONE_ARRAY:
        return "Array";
    case PONE_BOOL:
        return "Bool";
    case PONE_HASH:
        return "Hash";
    case PONE_CODE:
        return "Code";
    case PONE_OPAQUE:
        return "Opaque";
    case PONE_OBJ: {
        assert(pone_alive(val));
        pone_val* klass = pone_what(world, val);
        assert(pone_alive(klass));
        pone_val* name = pone_obj_get_ivar(world, val, "$!name");
        return pone_str_ptr(pone_str_c_str(world, pone_stringify(world, name)));
    }
    case PONE_LEX:
        return "Lex";
    }
    abort();
}

pone_val* pone_find_method(pone_world* world, pone_val* obj, const char* name) {
    assert(pone_alive(obj));

    pone_val* klass = pone_what(world, obj);
    assert(klass);
    if (klass == world->universe->class_module) {
        // Module
        pone_val* method = pone_obj_get_ivar(world, obj, name);
        if (pone_defined(method)) {
            return method;
        } else {
            pone_val* methods = pone_obj_get_ivar(world, klass, "$!methods");
            pone_val* method = pone_hash_at_key_c(world->universe, methods, name);
            if (pone_defined(method)) {
                return method;
            } else {
                return NULL;
            }
        }
    } else {
        // Normal object
        pone_val* methods = pone_obj_get_ivar(world, klass, "$!methods");
        assert(methods);
        assert(pone_type(methods) == PONE_HASH);
        pone_val* method = pone_hash_at_key_c(world->universe, methods, name);
        assert(method);
        if (pone_defined(method)) {
            return method;
        } else {
            return NULL;
        }
    }
}

// Usage: return pone_call_method(world, iter, "pull-one", 0);
pone_val* pone_call_method(pone_world* world, pone_val* obj, const char* method_name, int n, ...) {
    assert(obj);
#ifndef NDEBUG
    if (!pone_alive(obj)) {
        fprintf(stderr, "PONE BUG [%lx] value %p is already free'd\n", pthread_self(), obj);
        abort();
    }
#endif

    pone_val* method = pone_find_method(world, obj, method_name);
    if (method) {
        if (pone_type(method) == PONE_CODE) {
            va_list args;

            va_start(args, n);

            pone_val* retval = pone_code_vcall(world, method, obj, n, args);

            va_end(args);
            return retval;
        } else {
            return method;
        }
    } else {
        pone_throw_str(world, "Method '%s' not found for invocant of class '%s'", method_name, pone_what_str_c(world, obj));
        abort();
    }
}

pone_val* pone_call_meta_method(pone_world* world, pone_val* obj, const char* method_name, int n, ...) {
    if (strcmp(method_name, "methods") == 0) {
        // .^methods
        pone_val* klass = pone_what(world, obj);
        pone_val* methods = pone_obj_get_ivar(world, klass, "$!methods");
        return pone_hash_keys(world, methods);
    } else {
        pone_throw_str(world, "Meta method '%s' not found for invocant of class '%s'", method_name, pone_what_str_c(world, obj));
        abort();
    }
}
