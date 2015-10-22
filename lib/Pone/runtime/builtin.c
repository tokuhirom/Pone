
pone_val*  pone_builtin_dd(pone_world* world, pone_val* val) {
    pone_dd(world, val);
    return &pone_undef_val;
}

pone_val*  pone_builtin_abs(pone_world* world, pone_val* val) {
    switch (pone_type(val)) {
    case PONE_INT: {
        int i = pone_int_val(val);
        if (i < 0) {
            return pone_mortalize(world, pone_new_int(world, -i));
        } else {
            return val;
        }
    }
    case PONE_STRING:
        break;
                   // TODO: NV
    }

    pone_die(world, "you can't call abs() for non-numeric value");
    abort();
}

pone_val* pone_builtin_print(pone_world* world, pone_val* val) {
    pone_val* str = pone_str(world, val);
    fwrite(pone_string_ptr(str), sizeof(char), pone_string_len(str), stdout);
    return &pone_undef_val;
}

pone_val* pone_builtin_say(pone_world* world, pone_val* val) {
    pone_builtin_print(world, val);
    fwrite("\n", sizeof(char), 1, stdout);
    return &pone_undef_val;
}

pone_val* pone_builtin_elems(pone_world* world, pone_val* val) {
    return pone_mortalize(world, pone_new_int(world, pone_elems(world, val)));
}
