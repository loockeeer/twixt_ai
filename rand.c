#include <stdint.h>
#include "rand.h"
// Credit : https://dotat.at/@/2023-06-21-pcg64-dxsm.html

uint64_t random_pull(pcg64_t *rng) {
    /* cheap (half-width) multiplier */
    const uint64_t mul = 15750249268501108917ULL;
    /* linear congruential generator */
    __uint128_t state = rng->state;
    rng->state = state * mul + rng->inc;
    /* DXSM (double xor shift multiply) permuted output */
    uint64_t hi = (uint64_t)(state >> 64);
    uint64_t lo = (uint64_t)(state | 1);
    hi ^= hi >> 32;
    hi *= mul;
    hi ^= hi >> 48;
    hi *= lo;
    return(hi);
}

pcg64_t random_seed(pcg64_t rng) {
    /* must ensure rng.inc is odd */
    rng.inc = (rng.inc << 1) | 1;
    rng.state += rng.inc;
    random_pull(&rng);
    return(rng);
}
// End credit