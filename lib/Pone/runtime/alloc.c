
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
        case PONE_ARRAY: {
            pone_ary* a=(pone_ary*)val;
            size_t l = pone_ary_elems(val);
            for (int i=0; i<l; ++i) {
                pone_refcnt_dec(world, a->a[i]);
            }
            pone_free(world, a->a);
            break;
        }
        case PONE_HASH: {
            pone_hash* h=(pone_hash*)val;
            const char* k;
            pone_val* v;
            kh_foreach(h->h, k, v, {
                pone_free(world, (void*)k); // k is strdupped.
                pone_refcnt_dec(world, v);
            });
            kh_destroy(str, h->h);
            break;
        }
        }
        pone_free(world, val);
    }
}

