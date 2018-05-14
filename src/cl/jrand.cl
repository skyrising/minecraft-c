#ifndef __JRAND_CL
#define __JRAND_CL

#define RANDOM_MULTIPLIER 0x5DEECE66DUL
#define RANDOM_ADDEND 0xBUL
#define RANDOM_MASK (1UL << 48) - 1

// new Random(seed)
#define get_random(seed) ((ulong) ((seed ^ RANDOM_MULTIPLIER) & RANDOM_MASK))

// Random::next(bits)
uint random_next (ulong *random, int bits) {
  *random = (*random * RANDOM_MULTIPLIER + RANDOM_ADDEND) & RANDOM_MASK;
  return (uint)(*random >> (48 - bits));
}

// Random::nextInt(bound)
uint random_next_int (ulong *random, uint bound) {
  uint r = random_next(random, 31);
  uint m = bound - 1;
  if ((bound & m) == 0) {
      r = (uint)((bound * (ulong)r) >> 31);
  } else {
      for (int u = r;
           u - (r = u % bound) + m < 0;
           u = random_next(random, 31));
  }
  return r;
}

#endif
