#include "pone.h"
#include "pone_module.h"
#include "../3rd/picohttpparser/picohttpparser.h"

PONE_FUNC(meth_parse_request) {
    pone_val* buf;

    PONE_ARG("http/parser.parse_request", "o", &buf);

    const char* method, *path;
    int minor_version;
    struct phr_header headers[100];
    size_t prevbuflen = 0, method_len, path_len;
    size_t num_headers = sizeof(headers) / sizeof(headers[0]);

    int pret = phr_parse_request(
        pone_str_ptr(buf),
        pone_str_len(buf),
        &method, &method_len, &path, &path_len,
        &minor_version, headers, &num_headers, prevbuflen);
    if (pret > 0) {
        // TODO use list
        pone_val* env = pone_map_new(world);
#define SET(key, val)                                                    \
    pone_map_assign_key(world, env,                                      \
                        pone_str_new_const(world, key, sizeof(key) - 1), \
                        val)
        SET("REQUEST_METHOD", pone_str_new_strdup(world, method, method_len));
        SET("REQUEST_URI", pone_str_new_strdup(world, path, path_len));
        SET("SCRIPT_NAME", pone_str_new_const(world, "", 0));
        return pone_ary_new(world, 2, pone_int_new(world, pret), env);
    } else {
        return pone_ary_new(world, 1, pone_int_new(world, pret));
    }
}

void PONE_DLL_http_parser(pone_world* world, pone_val* module) {
    {
        pone_val* code = pone_code_new(world, meth_parse_request);
        pone_module_put(world, module, "parse_request", code);
    }
}
