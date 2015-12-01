#ifndef PONE_UTF8_H_
#define PONE_UTF8_H_

#include "oniguruma.h"

static inline int utf8_next_char_len(char* s) {
    return ONIGENC_MBC_ENC_LEN(ONIG_ENCODING_UTF8, (OnigUChar*)s);
}

static inline unsigned int utf8_code(char* s, char* end) {
    return ONIGENC_MBC_TO_CODE(ONIG_ENCODING_UTF8, (OnigUChar*)s, (OnigUChar*)end);
}

#endif

