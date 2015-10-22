
inline int pone_int_val(pone_val* val) {
    assert(pone_type(val) == PONE_INT);
    return ((pone_int*)val)->i;
}

pone_val* pone_new_int(pone_world* world, int i) {
    pone_int* iv = (pone_int*)pone_malloc(world, sizeof(pone_int));
    iv->refcnt = 1;
    iv->type   = PONE_INT;
    iv->i = i;
    return (pone_val*)iv;
}

