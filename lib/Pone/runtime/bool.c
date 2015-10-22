#include <stdbool.h>

static pone_bool pone_true_val = { -1, PONE_BOOL, 0, true };
static pone_bool pone_false_val = { -1, PONE_BOOL, 0, false };

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

