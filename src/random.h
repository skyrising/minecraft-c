#ifndef __RANDOM_H
#define __RANDOM_H
#include <inttypes.h>

typedef uint64_t Random;

#define RANDOM_MULTIPLIER 0x5DEECE66DULL
#define RANDOM_ADDEND 0xBULL
#define RANDOM_MASK (1ULL << 48) - 1

// new Random(seed)
#define get_random(seed) ((Random) (uint64_t) ((seed ^ RANDOM_MULTIPLIER) & RANDOM_MASK))

// Random::next(bits)
static inline uint32_t random_next (Random *random, int bits) {
  *random = (*random * RANDOM_MULTIPLIER + RANDOM_ADDEND) & RANDOM_MASK;
  return (uint32_t)(*random >> (48 - bits));
}

// Random::nextInt(bound)
static inline uint32_t random_next_int (Random *random, uint32_t bound) {
  uint32_t r = random_next(random, 31);
  uint32_t m = bound - 1;
  if ((bound & m) == 0) {
      r = (uint32_t)((bound * (uint64_t)r) >> 31);
  } else {
      for (int u = r;
           u - (r = u % bound) + m < 0;
           u = random_next(random, 31));
  }
  return r;
}

#endif
