pone_val* pone_new_num(pone_world* world, double n) {
    pone_num* nv = (pone_num*)pone_malloc(world, sizeof(pone_num));
    nv->refcnt = 1;
    nv->type   = PONE_NUM;
    nv->n = n;
    return (pone_val*)nv;
}

inline double pone_num_val(pone_val* val) {
    assert(pone_type(val) == PONE_NUM);
    return ((pone_num*)val)->n;
}

