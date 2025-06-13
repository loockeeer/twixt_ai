#ifndef RAND_H
#define RAND_H

typedef struct pcg64 {
    __uint128_t state, inc;
} pcg64_t;
uint64_t random_pull(pcg64_t *rng);
pcg64_t random_seed(pcg64_t rng);

#endif //RAND_H
