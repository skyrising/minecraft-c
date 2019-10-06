#ifndef __SEED_CRACKING_CL
#define __SEED_CRACKING_CL

#include <jrand.cl>

#define BASE_KERNEL(name, get_seed, predicate) \
__kernel void (name)(ulong offset, ulong stride, __global __write_only ulong *seeds, __global __write_only ushort *ret) {\
  size_t id = get_global_id(0);\
  ushort count = 0;\
  ulong seed_base = (offset + id) * stride;\
  for (ulong i = 0; i < stride; i++) {\
    ulong seed = random_world_seed(seed_base | i);\
    if (!(predicate)(seed)) continue;\
    seeds[id] = seed;\
    count++;\
  }\
  ret[id] = count;\
}

static inline ulong random_world_seed(ulong base) {
  Random r;
  set_internal_seed(&r, base);
  return random_next_long(&r);
}

#define RANDOM_WORLD_SEED_KERNEL(name, predicate) BASE_KERNEL(name, random_next_long, predicate)

static inline ulong direct_seed(ulong base) {
  return base;
}

#define STRUCTURE_SEED_KERNEL(name, predicate) BASE_KERNEL(name, direct_seed, predicate)

static inline void set_structure_seed (Random *random, long seed, int x, int z, int salt) {
  return set_seed(random, x * 341873128712L + z * 132897987541L + seed + salt);
}

static inline void set_generic_structure_seed (Random *random, long seed, int x, int z) {
  set_seed(random, seed);
  long xMul = random_next_long(random);
  long zMul = random_next_long(random);
  set_seed(random, (xMul * x) ^ (zMul * z) ^ seed);
}

static inline bool temple_at(ulong seed, int salt, int x, int z) {
  Random r;
  set_structure_seed(&r, seed, x >> 5, z >> 5, salt);
  if (random_next_int_24(&r) != (x & 0x1f)) return 0;
  if (random_next_int_24(&r) != (z & 0x1f)) return 0;
  return 1;
}

static inline bool monument_at(ulong seed, int x, int z) {
  Random r;
  set_structure_seed(&r, seed, x >> 5, z >> 5, 10387313);
  if (((random_next_int(&r, 27) + random_next_int(&r, 27)) >> 1) != (x & 0x1f)) return 0;
  if (((random_next_int(&r, 27) + random_next_int(&r, 27)) >> 1) != (z & 0x1f)) return 0;
  return 1;
}

static inline bool buried_treasure_at(long seed, int x, int z) {
  Random r;
  set_structure_seed(&r, seed, x, z, 10387320);
  return random_next(&r, 24) < 167773;
}

static inline bool mineshaft_at(long seed, int x, int z) {
  Random r;
  set_generic_structure_seed(&r, seed, x, z);
  if ((random_next(&r, 26)) > 268435) return 0;
  if ((random_next(&r, 27)) > 61203284) return 0;
  return 1;
}

#endif
