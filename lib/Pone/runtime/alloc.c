
// TODO: implement memory pool
void* pone_malloc(pone_world* world, size_t size) {
    void* p = malloc(size);
    if (!p) {
        fprintf(stderr, "Cannot allocate memory\n");
        exit(1);
    }
    memset(p, 0, size);
    return p;
}

pone_val* pone_obj_alloc(pone_world* world, pone_t type) {
    pone_val* val = (pone_val*)pone_malloc(world, sizeof(pone_val));
    val->as.basic.refcnt = 1;
    val->as.basic.type = type;
    return val;
}

void pone_free(pone_world* world, void* p) {
    free(p);
}

const char* pone_strdup(pone_world* world, const char* src, size_t size) {
    char* p = (char*)malloc(size+1);
    if (!p) {
        fprintf(stderr, "Cannot allocate memory\n");
        exit(1);
    }
    memcpy(p, src, size);
    p[size] = '\0';
    return p;
}

inline void pone_refcnt_inc(pone_world* world, pone_val* val) {
#ifdef TRACE_REFCNT
    printf("pone_refcnt_inc: world:%X val:%X\n", world, val);
#endif
    assert(val != NULL);

    val->as.basic.refcnt++;
}

// decrement reference count
inline void pone_refcnt_dec(pone_world* world, pone_val* val) {
#ifdef TRACE_REFCNT
    printf("refcnt_dec: %X refcnt:%d type:%s\n", val, val->refcnt, pone_what_str_c(val));
    pone_dd(world, val);
#endif

    if (val->as.basic.flags & PONE_FLAGS_GLOBAL) {
        return;
    }

    assert(val != NULL);
#ifndef NDEBUG
    if (val->as.basic.refcnt <= 0) { 
        pone_dd(world, val);
        assert(val->as.basic.refcnt > 0);
    }
#endif


    val->as.basic.refcnt--;
    if (val->as.basic.refcnt == 0) {
        switch (pone_type(val)) {
        case PONE_STRING:
            pone_str_free(world, val);
            break;
        case PONE_ARRAY:
            pone_ary_free(world, val);
            break;
        case PONE_HASH:
            pone_hash_free(world, val);
            break;
        case PONE_CODE:
            pone_code_free(world, val);
            break;
        case PONE_INT: // don't need to free heap
        case PONE_NUM:
            break;
        case PONE_NIL:
        case PONE_BOOL:
            abort(); // should not reach here.
        }
#ifdef TRACE_REFCNT
        memset(val, 0, sizeof(pone_val));
#endif
        pone_free(world, val);
    }
}

