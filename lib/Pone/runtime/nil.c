#include "pone.h" /* PONE_INC */

static pone_val pone_nil_val = { -1, PONE_NIL, PONE_FLAGS_GLOBAL };

pone_val* pone_nil() {
    return &pone_nil_val;
}

