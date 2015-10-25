#include "pone.h" /* PONE_INC */

static pone_val pone_nil_val = { -1, PONE_NIL, 0 };

inline pone_val* pone_nil() {
    return &pone_nil_val;
}

