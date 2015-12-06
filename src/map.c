#include "pone.h"
#include "pone_map.h"

static inline khash_t(val) * HASH(pone_val* v) {
    assert(pone_type(v) == PONE_MAP);
    return v->as.map.body->h;
}

void pone_map_mark(pone_val* val) {
    PONE_MAP_FOREACH(val, k, v, {
        pone_gc_mark_value(k);
        pone_gc_mark_value(v);
    });
}

pone_val* pone_map_copy(pone_world* world, pone_val* obj) {
    pone_val* retval = pone_map_new(world);
    PONE_MAP_FOREACH(obj, k, v, {
        pone_map_assign_key(world,
            retval,
            pone_val_copy(world, k),
            pone_val_copy(world, v));
    });
    return retval;
}

// TODO: delete key
// TODO: exists key

pone_val* pone_map_new(pone_world* world) {
    pone_val* hv = pone_obj_alloc(world, PONE_MAP);
    struct pone_hash_body* body = pone_malloc(world, sizeof(struct pone_hash_body));
    body->len = 0;
    body->h = kh_init(val);
    hv->as.map.body = body;
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
    kh_destroy(val, val->as.map.body->h);
    pone_free(world, val->as.map.body);
}

pone_val* pone_map_at_key(pone_world* world, pone_val* self, pone_val* key) {
    assert(pone_type(self) == PONE_MAP);

    khint_t k = kh_get(val, HASH(self), key);
    if (k != kh_end(HASH(self))) {
        return kh_val(HASH(self), k);
    } else {
        return pone_nil();
    }
}

void pone_map_assign_key(pone_world* world, pone_val* hv, pone_val* key, pone_val* value) {
    if (pone_type(key) != PONE_STRING) {
        key = pone_stringify(world, key);
    }
    assert(pone_type(hv) == PONE_MAP);
    int ret;
    khint_t k = kh_put(val, HASH(hv), key, &ret);
    if (ret == -1) {
        fprintf(stderr, "[BUG] khash.h returns error: %s\n", pone_str_ptr(pone_str_c_str(world, key)));
        abort();
    }
    kh_val(hv->as.map.body->h, k) = value;
    hv->as.map.body->len++;
}

pone_int_t pone_map_size(pone_val* val) {
    assert(pone_type(val) == PONE_MAP);
    return val->as.map.body->len;
}

pone_val* pone_map_values(pone_world* world, pone_val* val) {
    assert(pone_type(val) == PONE_MAP);

    pone_val* retval = pone_ary_new(world, 0);

    PONE_MAP_FOREACH(val, k, v, {
        pone_ary_push(world, retval, v);
        (void)k;
    });

    return retval;
}

pone_val* pone_map_keys(pone_world* world, pone_val* val) {
    assert(pone_type(val) == PONE_MAP);

    pone_val* retval = pone_ary_new(world, 0);

    PONE_MAP_FOREACH(val, k, v, {
        pone_ary_push(world, retval, k);
        (void)v;
    });

    return retval;
}

PONE_FUNC(meth_hash_gist) {
    PONE_ARG("Map#gist", "");

    pone_val* retval = pone_str_new_strdup(world, "{", 1);

    int i = 10;
    PONE_MAP_FOREACH(self, k, v, {
        if (i-- < 0) {
            break;
        }
        pone_str_append(world, retval, k);
        pone_str_append_c(world, retval, "=>", strlen("=>"));
        pone_str_append(world, retval, v);
        pone_str_append_c(world, retval, ", ", strlen(", "));
    });
    if (i == 0) {
        pone_str_append_c(world, retval, "... ", strlen("... "));
    }

    pone_str_append_c(world, retval, ", ", strlen(", "));

    return retval;
}

PONE_FUNC(meth_hash_size) {
    PONE_ARG("Map#size", "");
    return pone_int_new(world, pone_map_size(self));
}

PONE_FUNC(meth_hash_keys) {
    PONE_ARG("Map#keys", "");
    return pone_map_keys(world, self);
}

PONE_FUNC(meth_hash_values) {
    PONE_ARG("Map#values", "");
    return pone_map_values(world, self);
}

PONE_FUNC(meth_hash_at_key) {
    pone_val* key;
    PONE_ARG("Map#AT-KEY", "o", &key);
    return pone_map_at_key(world, self, key);
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
    pone_add_method_c(world, universe->class_map, "keys", strlen("keys"), meth_hash_keys);
    pone_add_method_c(world, universe->class_map, "values", strlen("values"), meth_hash_values);
    pone_add_method_c(world, universe->class_map, "gist", strlen("gist"), meth_hash_gist);
    pone_add_method_c(world, universe->class_map, "ASSIGN-KEY", strlen("ASSIGN-KEY"), meth_hash_assign_key);
    pone_add_method_c(world, universe->class_map, "AT-KEY", strlen("AT-KEY"), meth_hash_at_key);
    pone_universe_set_global(world->universe, "Map", universe->class_map);
}
