#include "pone.h" /* PONE_INC */

static pone_nil_t pone_nil_val = { PONE_NIL, -1, PONE_FLAGS_GLOBAL };

pone_val* pone_nil() {
    return (pone_val*)&pone_nil_val;
}

