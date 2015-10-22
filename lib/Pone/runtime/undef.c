#include "pone.h" /* PONE_INC */

static pone_val pone_undef_val = { -1, PONE_UNDEF, 0 };

inline pone_val* pone_undef() {
    return &pone_undef_val;
}

