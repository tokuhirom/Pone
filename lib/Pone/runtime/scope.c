void pone_savetmps(pone_world* world) {
    // save original tmpstack_floor
    world->savestack[world->savestack_idx] = world->tmpstack_floor;
    ++world->savestack_idx;
    if (world->savestack_max+1 < world->savestack_idx) {
        // grow it
        world->savestack_max *= 2;
        size_t* ssp = realloc(world->savestack, sizeof(size_t)*world->savestack_max);
        if (!ssp) {
            pone_die(world, "Cannot allocate memory");
        }
        world->savestack = ssp;
    }

    // save current tmpstack_idx
    world->tmpstack_floor = world->tmpstack_idx;
}

void pone_push_scope(pone_world* world) {
    // create new lex scope
    lex_entry* new_lex = pone_malloc(world, sizeof(lex_entry));
    new_lex->map = kh_init(str);
    new_lex->parent = world->lex;
    world->lex = new_lex;
}

void pone_enter(pone_world* world) {
    pone_savetmps(world);
    pone_push_scope(world);
}

void pone_pop_scope(pone_world* world) {
    lex_entry* parent = world->lex->parent;
    {
        const char* k;
        pone_val* v;
        kh_foreach(world->lex->map, k, v, {
            pone_refcnt_dec(world, v);
        });
    }
    kh_destroy(str, world->lex->map);
    free(world->lex);
    world->lex = parent;
}

void pone_freetmps(pone_world* world) {
    // decrement refcnt for mortalized values
    while (world->tmpstack_idx > world->tmpstack_floor) {
        pone_refcnt_dec(world, world->tmpstack[world->tmpstack_idx-1]);
        --world->tmpstack_idx;
    }

    // restore tmpstack_floor
    world->tmpstack_floor = world->savestack[world->savestack_idx-1];

    // pop tmpstack_floor
    --world->savestack_idx;
}

void pone_leave(pone_world* world) {
    pone_freetmps(world);
    pone_pop_scope(world);
}

pone_val* pone_mortalize(pone_world* world, pone_val* val) {
    if (world->tmpstack_idx == world->tmpstack_max) {
        world->tmpstack_max *= 2;
        pone_val** ssp = realloc(world->tmpstack, sizeof(pone_val*)*world->tmpstack_max);
        if (!ssp) {
            pone_die(world, "Cannot allocate memory");
        }
        world->tmpstack = ssp;
    }
    world->tmpstack[world->tmpstack_idx] = val;
    ++world->tmpstack_idx;
    return val;
}

