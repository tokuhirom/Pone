#ifndef PONE_HASH_H_
#define PONE_HASH_H_

#include "pone.h"
#include "khash.h"

#define PONE_MAP_FOREACH(v, kvar, vvar, code)                  \
    assert(pone_type(v) == PONE_MAP);                          \
    khash_t(val)* h = v->as.map.body->h;                       \
    for (khint_t __i = kh_begin(h); __i != kh_end(h); ++__i) { \
        if (!kh_exist(h, __i))                                 \
            continue;                                          \
        pone_val*(kvar) = kh_key(h, __i);                      \
        pone_val*(vvar) = kh_val(h, __i);                      \
        code;                                                  \
    }

static inline khint_t pone_val_hash_func(pone_val* val) {
    assert(pone_type(val) == PONE_STRING);

    const char* s = pone_str_ptr(val);
    const char* end = s + pone_str_len(val);
    khint_t h = (khint_t) * s;
    if (h)
        for (++s; s != end; ++s)
            h = (h << 5) - h + (khint_t) * s;
    return h;
}

static inline bool pone_val_hash_equal(pone_val* a, pone_val* b) {
    assert(pone_type(a) == PONE_STRING);
    assert(pone_type(b) == PONE_STRING);
    return pone_str_len(a) == pone_str_len(b)
           && memcmp(pone_str_ptr(a), pone_str_ptr(b), pone_str_len(a)) == 0;
}

KHASH_INIT(val, struct pone_val*, struct pone_val*, 1, pone_val_hash_func, pone_val_hash_equal)

struct pone_hash_body {
    khash_t(val) * h;
    pone_int_t len;
};

#endif // PONE_HASH_H_
