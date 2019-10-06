#include <seed-cracking.cl>

inline bool is_valid_seed(ulong world_seed) {
  if (!buried_treasure_at(world_seed, -65, -51)) return 0;
  if (!buried_treasure_at(world_seed, -90, 16)) return 0;
  if (!temple_at(world_seed, 165745296, -15, 71)) return 0;
  if (!temple_at(world_seed, 14357617, 5, 33)) return 0;
  if (!monument_at(world_seed, 19, -61)) return 0;
  return 1;
}

RANDOM_WORLD_SEED_KERNEL(kern, is_valid_seed)
