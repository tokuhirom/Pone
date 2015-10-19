#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

typedef enum {
    PONE_UNDEF,
    PONE_INT,
    PONE_STRING
} pone_t;

typedef struct {
    int refcnt;
    pone_t type;
} head;

typedef struct {
    head head;
} pone_val;

typedef struct {
    head head;
} pone_undef;

// integer value
typedef struct {
    head head;
    int i;
} pone_int;

typedef struct {
    head head;
    const char* p;
    size_t len;
} pone_string;

static pone_val pone_undef_val = { { -1, PONE_UNDEF } };

pone_val* pone_new_str(const char*p, size_t len);
pone_val* pone_str(pone_val* val);
pone_t pone_type(pone_val* val);
size_t pone_int_val(pone_val* val);
const char* pone_string_ptr(pone_val* val);
size_t pone_string_len(pone_val* val);

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

inline pone_t pone_type(pone_val* val) {
    return val->head.type;
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

pone_val* pone_str_from_int(int i) {
    // INT_MAX=2147483647. "2147483647".elems = 10
    char buf[10+1];
    int size = snprintf(buf, 10+1, "%d", i);
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
    iv->head.refcnt = 1;
    iv->head.type   = PONE_INT;
    iv->i = i;
    return (pone_val*)iv;
}

pone_val* pone_new_str(const char*p, size_t len) {
    pone_string* pv = (pone_string*)pone_malloc(sizeof(pone_str));
    pv->head.refcnt = 1;
    pv->head.type = PONE_STRING;
    pv->p = pone_strdup(p, len);
    pv->len = len;
    return (pone_val*)pv;
}

pone_val* pone_new_int_mortal(int i) {
    return pone_val_2mortal(pone_new_int(i));
}

#ifdef PONE_TESTING

int main(int argc, char** argv) {
    pone_val* iv = pone_new_int(4963);
    pone_builtin_say(iv);

    pone_val* pv = pone_new_str("Hello, world!", strlen("Hello, world!"));
    pone_builtin_say(pv);
}

#endif
