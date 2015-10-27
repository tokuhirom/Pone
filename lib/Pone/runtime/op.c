pone_val* pone_get_lex(pone_world* world, const char* key) {
    pone_lex_t* lex = world->lex;
    while (lex != NULL) {
        khint_t kh = kh_get(str, lex->map, key);
        if (kh == kh_end(lex->map)) {
            lex = lex->parent;
            continue;
        }
        return kh_val(lex->map, kh);
    }
    fprintf(stderr, "unknown lexical variable: %s\n", key);
    abort();
}

pone_val* pone_assign(pone_world* world, int up, const char* key, pone_val* val) {
    pone_lex_t* lex = world->lex;
    for (int i=0; i<up; i++) {
        lex = lex->parent;
    }

    pone_refcnt_inc(world, val);
    int ret;
    khint_t k = kh_put(str, lex->map, key, &ret);
    if (ret == -1) {
        fprintf(stderr, "hash operation failed\n");
        abort();
    }
    if (ret == 0) { // the key is present in the hash table
        pone_refcnt_dec(world, kh_val(lex->map, k));
    }
    kh_val(lex->map, k) = val;

    return val;
}

void pone_dd(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
        case PONE_STRING:
            printf("(string: ");
            fwrite(pone_string_ptr(val), 1, pone_string_len(val), stdout);
            printf(")\n");
            break;
        case PONE_INT:
            printf("(int: refcnt:%d, %d)\n", pone_refcnt(val), pone_int_val(val));
            break;
        case PONE_NIL:
            printf("(undef)\n");
            break;
        case PONE_CODE:
            printf("(code)\n");
            break;
        default:
            abort();
    }
}


bool pone_so(pone_val* val) {
    switch (pone_type(val)) {
    case PONE_INT:
        return pone_int_val(val) != 0;
    case PONE_NUM:
        return pone_int_val(val) != 0;
    case PONE_STRING:
        return pone_string_len(val) != 0;
    case PONE_BOOL:
        return pone_bool_val(val);
    default:
        return true;
    }
}

void pone_die(pone_world* world, const char* str) {
    fprintf(stderr, "%s\n", str);
    exit(1);
}

int pone_to_int(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_NIL:
        pone_die(world, "Use of uninitialized value as integer");
        abort();
    case PONE_INT:
        return pone_int_val(val);
    case PONE_STRING: {
        char *end = (char*)pone_string_ptr(val) + pone_string_len(val);
        return strtol(pone_string_ptr(val), &end, 10);
    }
    case PONE_CODE:
        pone_die(world, "you can't convert CODE to integer");
    default:
        pone_die(world, "you can't convert this type to integer");
        abort();
    }
}

// TODO: support NV
pone_val* pone_add(pone_world* world, pone_val* v1, pone_val* v2) {
    int i1 = pone_to_int(world, v1);
    int i2 = pone_to_int(world, v2);
    return pone_mortalize(world, pone_new_int(world, i1 + i2));
}

// TODO: support NV
pone_val* pone_subtract(pone_world* world, pone_val* v1, pone_val* v2) {
    int i1 = pone_to_int(world, v1);
    int i2 = pone_to_int(world, v2);
    return pone_mortalize(world, pone_new_int(world, i1 - i2));
}

pone_val* pone_multiply(pone_world* world, pone_val* v1, pone_val* v2) {
    int i1 = pone_to_int(world, v1);
    int i2 = pone_to_int(world, v2);
    return pone_mortalize(world, pone_new_int(world, i1 * i2));
}

pone_val* pone_divide(pone_world* world, pone_val* v1, pone_val* v2) {
    int i1 = pone_to_int(world, v1);
    int i2 = pone_to_int(world, v2);
    return pone_mortalize(world, pone_new_int(world, i1 / i2)); // TODO: We should upgrade value to NV
}

size_t pone_elems(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_STRING:
        return pone_string_len(val);
    case PONE_ARRAY:
        return pone_ary_elems(val);
    case PONE_HASH:
        return pone_hash_elems(val);
    case PONE_NIL:
    case PONE_INT:
    case PONE_NUM:
    case PONE_BOOL:
    case PONE_CODE:
        return 1; // same as perl6
    }
}

const char* pone_what_str_c(pone_val* val) {
    switch (pone_type(val)) {
    case PONE_NIL:
        return "Any";
    case PONE_INT:
        return "Int";
    case PONE_NUM:
        return "Num";
    case PONE_STRING:
        return "Str";
    case PONE_ARRAY:
        return "Array";
    case PONE_BOOL:
        return "Bool";
    case PONE_HASH:
        return "Hash";
    case PONE_CODE:
        return "Code";
    }
}

