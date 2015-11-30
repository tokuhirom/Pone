#include "pone.h"

void pone_map_mark(pone_val* val) {
    const char* k;
    pone_val* v;
    kh_foreach(val->as.map.h, k, v, {
        pone_gc_mark_value(v);
    });
}

pone_val* pone_map_copy(pone_world* world, pone_val* obj) {
    pone_val* retval = pone_map_new(world);
    const char* k;
    pone_val* v;
    kh_foreach(obj->as.map.h, k, v, {
        pone_map_assign_key(world,
            retval,
            pone_str_new_strdup(world, k, strlen(k)),
            pone_val_copy(world, v));
    });
    return retval;
}

// TODO: delete key
// TODO: push key
// TODO: exists key

pone_val* pone_map_new(pone_world* world) {
    pone_val* hv = pone_obj_alloc(world, PONE_MAP);
    hv->as.map.h = kh_init(str);
    return hv;
}

pone_val* pone_map_assign_keys(pone_world* world, pone_val* hash, pone_int_t n, ...) {
    va_list list;

    va_start(list, n);
    // we can optimize in case of `{a => 3}`. we can omit mortalize.
    for (pone_int_t i = 0; i < n; i += 2) {
        pone_val* k = va_arg(list, pone_val*);
        pone_val* v = va_arg(list, pone_val*);
        pone_map_assign_key(world, hash, k, v);
    }
    va_end(list);

    return hash; // return itself
}

void pone_map_free(pone_world* world, pone_val* val) {
    pone_map* h = (pone_map*)val;
    const char* k;
    pone_val* v;
    kh_foreach(h->h, k, v, {
        pone_free(world->universe, (void*)k); // k is strdupped.
    });
    kh_destroy(str, h->h);
}

void pone_map_assign_key_c(pone_world* world, pone_val* hv, const char* key, pone_int_t key_len, pone_val* v) {
    assert(pone_type(hv) == PONE_MAP);
    int ret;
    char* ks = pone_strdup(world, key, key_len);
    khint_t k = kh_put(str, ((pone_map*)hv)->h, ks, &ret);
    if (ret == -1) {
        fprintf(stderr, "[BUG] khash.h returns error: %s\n", key);
        abort();
    } else if (ret == 0) {
        // the key is present in the hash table.
        pone_free(world->universe, ks);
    }
    kh_val(((pone_map*)hv)->h, k) = v;
    ((pone_map*)hv)->len++;
}

bool pone_map_exists_c(pone_world* world, pone_val* hash, const char* name) {
    assert(pone_type(hash) == PONE_MAP);
    assert(hash->as.map.h);

    khint_t k = kh_get(str, hash->as.map.h, name);
    if (k != kh_end(hash->as.map.h)) {
        return true;
    } else {
        return false;
    }
}

pone_val* pone_map_at_key_c(pone_universe* universe, pone_val* hash, const char* name) {
    assert(pone_type(hash) == PONE_MAP);
    assert(hash->as.map.h);

    khint_t k = kh_get(str, hash->as.map.h, name);
    if (k != kh_end(hash->as.map.h)) {
        return kh_val(hash->as.map.h, k);
    } else {
        return pone_nil();
    }
}

void pone_map_assign_key(pone_world* world, pone_val* hv, pone_val* k, pone_val* v) {
    k = pone_str_c_str(world, pone_stringify(world, k));
    pone_map_assign_key_c(world, hv, pone_str_ptr(k), pone_str_len(k), v);
}

pone_int_t pone_map_size(pone_val* val) {
    assert(pone_type(val) == PONE_MAP);
    return ((pone_map*)val)->len;
}

pone_val* pone_map_keys(pone_world* world, pone_val* val) {
    assert(pone_type(val) == PONE_MAP);

    pone_map* h = (pone_map*)val;

    pone_val* retval = pone_ary_new(world, 0);

    const char* k;
    pone_val* v;
    kh_foreach(h->h, k, v, {
        pone_ary_push(world->universe, retval, pone_str_new_strdup(world, k, strlen(k)));
    });

    return retval;
}

PONE_FUNC(meth_hash_size) {
    PONE_ARG("Map#size", "");
    return pone_int_new(world, pone_map_size(self));
}

PONE_FUNC(meth_hash_assign_key) {
    pone_val* key;
    pone_val* value;
    PONE_ARG("Map#ASSIGN-KEY", "oo", &key, &value);
    pone_map_assign_key(world, self, key, value);
    return value;
}

void pone_map_init(pone_world* world) {
    pone_universe* universe = world->universe;
    assert(universe->class_map == NULL);

    universe->class_map = pone_class_new(world, "Map", strlen("Map"));
    pone_add_method_c(world, universe->class_map, "size", strlen("size"), meth_hash_size);
    pone_add_method_c(world, universe->class_map, "ASSIGN-KEY", strlen("ASSIGN-KEY"), meth_hash_assign_key);
    pone_universe_set_global(world->universe, "Map", universe->class_map);
}
