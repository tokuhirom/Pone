#include "pone.h"
#include "pone_module.h"
#include "pone_exc.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

// see http://xorshift.di.unimi.it/xorshift128plus.c
uint64_t pone_random_uint64(pone_world* world) {
    uint64_t* s = world->random_key;
    if (s[0] == 0 && s[0] == 0) {
        int fd = open("/dev/urandom", O_RDONLY);
        if (fd == -1) {
            pone_throw_str(world, "cannot open /dev/urandom: %s",
                           strerror(errno));
        }

        if (read(fd, world->random_key, sizeof(uint64_t) * 2) != sizeof(uint64_t) * 2) {
            pone_throw_str(world, "cannot read bytes from /dev/urandom: %s",
                           strerror(errno));
        }
    }

    uint64_t s1 = s[0];
    const uint64_t s0 = s[1];
    s[0] = s0;
    s1 ^= s1 << 23; // a
    return (s[1] = (s1 ^ s0 ^ (s1 >> 17) ^ (s0 >> 26))) + s0; // b, c
}

// Generate (A <= N <= B) random number.
// ref. http://www.sat.t.u-tokyo.ac.jp/~omi/random_variables_generation.html#Prepare_MT
int64_t pone_random_int_range(pone_world* world, int64_t a, int64_t b) {
    uint64_t i = pone_random_uint64(world);
    return i % (b - a + 1) + a;
}

PONE_FUNC(meth_random) {
    uint64_t i = pone_random_uint64(world);
    return pone_num_new(world, (double)i / UINT64_MAX);
}

PONE_FUNC(meth_rand_int) {
    pone_int_t A = 0;
    pone_int_t B = UINT_MAX;
    PONE_ARG("random.rand_int", "ii", &A, &B);
    return pone_int_new(world, pone_random_int_range(world, A, B));
}

void pone_random_init(pone_world* world) {
    pone_val* module = pone_module_new(world, "random");
    pone_module_put(world, module, "random", pone_code_new_c(world, meth_random));
    pone_module_put(world, module, "rand_int", pone_code_new_c(world, meth_rand_int));
    pone_universe_set_global(world->universe, "random", module);
}
