#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef enum {
    PONE_UNDEF,
    PONE_INT,
    PONE_NUM,
    PONE_STRING
} pone_t;

#define PONE_HEAD \
    int refcnt; \
    pone_t type;

typedef struct {
    PONE_HEAD;
} pone_val;

typedef struct {
    PONE_HEAD;
} pone_undef;

// integer value
typedef struct {
    PONE_HEAD;
    int i;
} pone_int;

typedef struct {
    PONE_HEAD;
    int i;
} pone_number;

typedef struct {
    PONE_HEAD;
    const char* p;
    size_t len;
} pone_string;

typedef struct {
} pone_world;

static pone_val pone_undef_val = { -1, PONE_UNDEF };

pone_val* pone_new_str(const char*p, size_t len);
pone_val* pone_str(pone_val* val);
pone_t pone_type(pone_val* val);
size_t pone_int_val(pone_val* val);
const char* pone_string_ptr(pone_val* val);
size_t pone_string_len(pone_val* val);
pone_val* pone_new_int_mortal(int i);
void* pone_malloc(size_t size);
void pone_free(void* ptr);
void pone_die(const char* str);

pone_world* pone_new_world() {
    // we can't use pone_malloc yet.
    pone_world* world = malloc(sizeof(pone_world));
    if (!world) {
        fprintf(stderr, "Cannot make world\n");
    }
    return world;
}

void pone_destroy_world(pone_world* world) {
    free(world);
}

void pone_dd(pone_val* val) {
    switch (pone_type(val)) {
        case PONE_STRING:
            printf("(string: ");
            fwrite(pone_string_ptr(val), 1, pone_string_len(val), stdout);
            printf(")\n");
            break;
        case PONE_INT:
            printf("(int: %d)\n", pone_int_val(val));
            break;
        case PONE_UNDEF:
            printf("(undef)\n");
            break;
        default:
            abort();
    }
}

pone_val*  pone_builtin_dd(pone_val* val) {
    pone_dd(val);
    return &pone_undef_val;
}

pone_val*  pone_builtin_abs(pone_val* val) {
    switch (pone_type(val)) {
    case PONE_INT: {
        int i = pone_int_val(val);
        if (i < 0) {
            return pone_new_int_mortal(-i);
        } else {
            return val;
        }
    }
                   // TODO: NV
    }

    pone_die("you can't call abs() for non-numeric value");
}

inline pone_t pone_type(pone_val* val) {
    return val->type;
}

inline const char* pone_string_ptr(pone_val* val) {
    assert(pone_type(val) == PONE_STRING);
    return ((pone_string*)val)->p;
}

inline size_t pone_string_len(pone_val* val) {
    assert(pone_type(val) == PONE_STRING);
    return ((pone_string*)val)->len;
}

inline size_t pone_int_val(pone_val* val) {
    assert(pone_type(val) == PONE_INT);
    return ((pone_int*)val)->i;
}

void pone_die(const char* str) {
    fprintf(stderr, "%s\n", str);
    exit(1);
}

int pone_to_int(pone_val* val) {
    switch (pone_type(val)) {
    case PONE_UNDEF:
        pone_die("Use of uninitialized value as integer");
        break;
    case PONE_INT:
        return pone_int_val(val);
    case PONE_STRING: {
        char *end = (char*)pone_string_ptr(val) + pone_string_len(val);
        return strtol(pone_string_ptr(val), &end, 10);
    }
    default:
        abort();
    }
}

// TODO: support NV
pone_val* pone_add(pone_val* v1, pone_val* v2) {
    int i1 = pone_to_int(v1);
    int i2 = pone_to_int(v2);
    return pone_new_int_mortal(i1 + i2);
}

// TODO: support NV
pone_val* pone_subtract(pone_val* v1, pone_val* v2) {
    int i1 = pone_to_int(v1);
    int i2 = pone_to_int(v2);
    return pone_new_int_mortal(i1 - i2);
}

pone_val* pone_multiply(pone_val* v1, pone_val* v2) {
    int i1 = pone_to_int(v1);
    int i2 = pone_to_int(v2);
    return pone_new_int_mortal(i1 * i2);
}

pone_val* pone_divide(pone_val* v1, pone_val* v2) {
    int i1 = pone_to_int(v1);
    int i2 = pone_to_int(v2);
    return pone_new_int_mortal(i1 / i2); // TODO: We should upgrade value to NV
}

pone_val* pone_str_from_int(int i) {
    // INT_MAX=2147483647. "2147483647".elems = 10
    char buf[11+1];
    int size = snprintf(buf, 11+1, "%d", i);
    return pone_new_str(buf, size);
}

pone_val* pone_str(pone_val* val) {
    switch (pone_type(val)) {
    case PONE_UNDEF:
        return pone_new_str("(undef)", strlen("(undef)"));
    case PONE_INT:
        return pone_str_from_int(pone_int_val(val));
    case PONE_STRING:
        return val;
    default:
        abort();
    }
}

pone_val* pone_builtin_print(pone_val* val) {
    pone_val* str = pone_str(val);
    fwrite(pone_string_ptr(str), sizeof(char), pone_string_len(str), stdout);
    return &pone_undef_val;
}

pone_val* pone_builtin_say(pone_val* val) {
    pone_builtin_print(val);
    fwrite("\n", sizeof(char), 1, stdout);
    return &pone_undef_val;
}

// TODO: implement memory pool
void* pone_malloc(size_t size) {
    void* p = malloc(size);
    if (!p) {
        fprintf(stderr, "Cannot allocate memory\n");
        exit(1);
    }
    memset(p, 0, size);
    return p;
}

void pone_free(void* p) {
    free(p);
}

const char* pone_strdup(const char* src, size_t size) {
    void* p = malloc(size);
    if (!p) {
        fprintf(stderr, "Cannot allocate memory\n");
        exit(1);
    }
    memcpy(p, src, size);
    return p;
}

pone_val* pone_val_2mortal(pone_val* sv) {
    return sv; // TODO: mortalize
}

pone_val* pone_new_int(int i) {
    pone_int* iv = (pone_int*)pone_malloc(sizeof(pone_int));
    iv->refcnt = 1;
    iv->type   = PONE_INT;
    iv->i = i;
    return (pone_val*)iv;
}

pone_val* pone_new_str(const char*p, size_t len) {
    pone_string* pv = (pone_string*)pone_malloc(sizeof(pone_str));
    pv->refcnt = 1;
    pv->type = PONE_STRING;
    pv->p = pone_strdup(p, len);
    pv->len = len;
    return (pone_val*)pv;
}

pone_val* pone_new_int_mortal(int i) {
    return pone_val_2mortal(pone_new_int(i));
}

#ifdef PONE_TESTING

int main(int argc, char** argv) {
    pone_world* world = pone_new_world();

    pone_val* iv = pone_new_int(4963);
    pone_builtin_say(iv);

    {
        pone_val* iv1 = pone_new_int(4963);
        pone_val* iv2 = pone_new_int(5963);
        pone_val* result = pone_add(iv1, iv2);
        pone_builtin_say(result);
    }

    {
        pone_val* iv1 = pone_new_int(4649);
        pone_val* iv2 = pone_new_int(5963);
        pone_val* result = pone_subtract(iv1, iv2);
        pone_builtin_say(result);
    }

    pone_val* pv = pone_new_str("Hello, world!", strlen("Hello, world!"));
    pone_builtin_say(pv);

    pone_destroy_world(world);
}

#endif
