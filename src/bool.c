#include "pone.h" /* PONE_INC */
#include <stdbool.h>

pone_bool pone_true_val = { PONE_BOOL, -1, PONE_FLAGS_GLOBAL, true };
pone_bool pone_false_val = { PONE_BOOL, -1, PONE_FLAGS_GLOBAL, false };

inline bool pone_bool_val(pone_val* val) {
    assert(pone_type(val) == PONE_BOOL);
    return ((pone_bool*)val)->b;
}

inline pone_val* pone_true() {
    return (pone_val*)&pone_true_val;
}

inline pone_val* pone_false() {
    return (pone_val*)&pone_false_val;
}

