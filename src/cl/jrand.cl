#ifndef __JRAND_CL
#define __JRAND_CL

// #define JRAND_DOUBLE
#define JRAND_SMOL

#define RANDOM_MULTIPLIER_LONG 0x5DEECE66DUL

#ifdef JRAND_DOUBLE
#define Random double
#define RANDOM_MULTIPLIER 0x5DEECE66Dp-48
#define RANDOM_ADDEND 0xBp-48

inline void set_internal_seed(Random *random, ulong state) {
  *random = (state * 0x1p-48);
}

inline uint random_next (Random *random, int bits) {
  *random = trunc(fma(*random, RANDOM_MULTIPLIER, RANDOM_ADDEND) * RANDOM_SCALE);
  return (uint)((ulong)(*random / RANDOM_SCALE) >> (48 - bits));
}

#else
#ifdef JRAND_SMOL

typedef struct _Random {
  ushort high;
  ushort mid;
  ushort low;
} Random;

#define RANDOM_MULTIPLIER_HIGH 0x0005U
#define RANDOM_MULTIPLIER_MID 0xdeecU
#define RANDOM_MULTIPLIER_LOW 0xe66dU
#define RANDOM_ADDEND 0xbU


inline void set_internal_seed(Random *random, ulong state) {
  random->high = (ushort) (state >> 32);
  random->mid = (ushort) (state >> 16);
  random->low = (ushort) state;
}

// Random::next(bits)
inline uint random_next (Random *random, int bits) {
  ushort a32 = random->high;
  ushort a16 = random->mid;
  ushort a00 = random->low;
  
  uint c32 = 0, c16 = 0, c00 = 0;
  c00 += a00 * RANDOM_MULTIPLIER_LOW;
  c16 += c00 >> 16;
  c00 &= 0xFFFF;
  c16 += a16 * RANDOM_MULTIPLIER_LOW;
  c32 += c16 >> 16;
  c16 &= 0xFFFF;
  c16 += a00 * RANDOM_MULTIPLIER_MID;
  c32 += c16 >> 16;
  c16 &= 0xFFFF;
  c32 += a32 * RANDOM_MULTIPLIER_LOW + a16 * RANDOM_MULTIPLIER_MID + a00 * RANDOM_MULTIPLIER_HIGH;
  c32 &= 0xFFFF;
    
  c00 += RANDOM_ADDEND;
  c16 += c00 >> 16;
  c00 &= 0xFFFF;
  c32 += c16 >> 16;
  
  c16 &= 0xffff;
  c32 &= 0xffff;
  
  random->high = (ushort) c32;
  random->mid = (ushort) c16;
  random->low = (ushort) c00;
  
  int shift = 48 - bits;
  if (shift >= 32) return c32 >> (shift - 32);
  if (shift >= 16) return (c32 << (32 - shift)) | (c16 >> (shift - 16));
  c00 &= 0xffff;
  c00 |= c16 << 16;
  return (c32 << (32 - shift)) | (c00 >> shift);
}

#else

#define Random ulong
#define RANDOM_MULTIPLIER RANDOM_MULTIPLIER_LONG
#define RANDOM_ADDEND 0xBUL
#define RANDOM_MASK (1UL << 48) - 1

inline void set_internal_seed(Random *random, ulong state) {
  *random = state;
}

// Random::next(bits)
inline uint random_next (Random *random, int bits) {
  *random = (*random * RANDOM_MULTIPLIER + RANDOM_ADDEND) & RANDOM_MASK;
  return (uint)(*random >> (48 - bits));
}
#endif // ~JRAND_SMOL
#endif // ~JRAND_DOUBLE

// new Random(seed)
#define set_seed(random, seed) set_internal_seed(random, (seed) ^ RANDOM_MULTIPLIER_LONG)

// Random::nextInt(bound)
inline uint random_next_int (Random *random, uint bound) {
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

inline long random_next_long (Random *random) {
  return (((long)random_next(random, 32)) << 32) + random_next(random, 32);
}

inline uint random_next_int_no_pow2(Random *random, uint bound) {
    int r = random_next(random, 31);
    int m = bound - 1;
    
    for (int u = r;
        u - (r = u % bound) + m < 0;
        u = random_next(random, 31));
      /*  
    uint u = r;
    r = u % bound;
    while (u - r + m < 0) {
      u = random_next(random, 31);
      r = u % bound;
    }*/
    return r;
}

inline uint random_next_int_24(Random *random) {
    return random_next_int(random, 24); /*
    uint u = random_next(random, 31);
    uint r = u % 24;
    while (u - r + 23 < 0) {
      u = random_next(random, 31);
      r = u % 24;
    }
    return r; */
}

inline uint random_next_int_pow2(Random *random, uint boundLog2) {
  return random_next(random, 31 - boundLog2);
}

#endif
