#include <stdbool.h>

static pone_bool pone_true_val = { -1, PONE_BOOL, PONE_FLAGS_GLOBAL, true };
static pone_bool pone_false_val = { -1, PONE_BOOL, PONE_FLAGS_GLOBAL, false };

inline bool pone_bool_val(pone_val* val) {
    assert(pone_type(val) == PONE_BOOL);
    return ((pone_bool*)val)->b;
}

pone_val* pone_true() {
    return (pone_val*)&pone_true_val;
}

pone_val* pone_false() {
    return (pone_val*)&pone_false_val;
}

