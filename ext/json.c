#include "pone.h"
#include "pone_module.h"
#include "pone_exc.h"
#include "pone_map.h"
#include "utf8.h"

static void stringify(pone_world* world, pone_val* buffer, pone_val* val) {
#define APPEND(s) pone_str_append_c(world, buffer, s, strlen(s))
    switch (pone_type(val)) {
    case PONE_NIL:
        pone_str_append_c(world, buffer, "null", sizeof("null") - 1);
        break;
    case PONE_INT:
        pone_str_append(world, buffer, val);
        break;
    case PONE_NUM:
        pone_str_append(world, buffer, val);
        break;
    case PONE_STRING:
        if (!(pone_flags(val) & PONE_FLAGS_STR_UTF8)) {
            pone_throw_str(world, "You can't make json by bytes");
        }

        // TODO escape chars
        pone_str_append_c(world, buffer, "\"", sizeof("\"") - 1);
        char* p = pone_str_ptr(val);
        char* end = p + pone_str_len(val);
        while (p != end) {
            unsigned int code = utf8_code(p, end);
            if (('a' <= code && code <= 'z')
                || ('A' <= code && code <= 'Z')
                || ('0' <= code && code <= '9')) {
                pone_str_append_c(world, buffer, p, 1);
            } else {
                pone_str_appendf(world, buffer, "\\u%04d", code);
            }
            p += utf8_next_char_len(p);
        }
        pone_str_append_c(world, buffer, "\"", sizeof("\"") - 1);
        break;
    case PONE_ARRAY: {
        APPEND("[");
        pone_int_t l = pone_ary_size(val);
        if (l >= 1) {
            stringify(world, buffer, pone_ary_at_pos(val, 0));
        }
        for (pone_int_t i = 1; i < l; ++i) {
            APPEND(",");
            stringify(world, buffer, pone_ary_at_pos(val, i));
        }
        APPEND("]");
        break;
    }
    case PONE_BOOL: {
        if (pone_so(val)) {
            APPEND("true");
        } else {
            APPEND("false");
        }
        break;
    }
    case PONE_MAP: {
        APPEND("{");
        PONE_MAP_FOREACH(val, k, v, {
            stringify(world, buffer, k);
            APPEND(":");
            stringify(world, buffer, v);
            APPEND(",");
        });
        if (val->as.map.body->len > 0) {
            buffer->as.str.len--; // remove trailing comma
        }
        APPEND("}");
        break;
    }
    case PONE_CODE:
        pone_throw_str(world, "You can't serialize code object");
    case PONE_OBJ:
        pone_throw_str(world, "You can't serialize object");
    case PONE_LEX:
        pone_throw_str(world, "You can't serialize lex object");
    case PONE_OPAQUE:
        pone_throw_str(world, "You can't serialize opaque object");
    }
}

PONE_FUNC(meth_stringify) {
    pone_val* val;
    PONE_ARG("json.stringify", "o", &val);

    pone_val* buffer = pone_str_new_strdup(world, "", 0);
    stringify(world, buffer, val);
    return buffer;
}

void PONE_DLL_json(pone_world* world, pone_val* module) {
    {
        pone_val* code = pone_code_new_c(world, meth_stringify);
        pone_module_put(world, module, "stringify", code);
    }
}
