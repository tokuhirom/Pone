
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
    assert(val != NULL);

    val->refcnt++;
}

// decrement reference count
inline void pone_refcnt_dec(pone_world* world, pone_val* val) {
    assert(val != NULL);

    val->refcnt--;
    if (val->refcnt == 0) {
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
        }
        pone_free(world, val);
    }
}

