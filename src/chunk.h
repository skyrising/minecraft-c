#ifndef __CHUNK_H
#define __CHUNK_H
#include <inttypes.h>
#include "random.h"

Random get_random_with_seed (uint64_t world_seed, int64_t x, int64_t z, uint64_t seed);
int is_slime_chunk (uint64_t seed, int64_t x, int64_t z);

#endif
