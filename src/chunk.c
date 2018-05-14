#include "chunk.h"

Random get_random_with_seed (uint64_t world_seed, int64_t x, int64_t z, uint64_t seed) {
  return get_random(world_seed + (int64_t)(x * x * 4987142) + (int64_t)(x * 5947611) + (int64_t)(z * z) * 4392871L + (int64_t)(z * 389711) ^ seed);
}

int is_slime_chunk (uint64_t seed, int64_t x, int64_t z) {
  Random r = get_random_with_seed(seed, x, z, 987234911UL);
  return random_next_int(&r, 10) == 0;
}
