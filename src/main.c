#include <stdlib.h>
#include <stdio.h>
#include "chunk.h"
#include "random.h"

#define LOG_INTERVAL (1 << 30)
#define LOG_SUFFIX "G"

void show_area (uint64_t seed, int64_t cx, int64_t cz) {
  for (int64_t z = cz - 5; z < cz + 5; z++) {
    for (int64_t x = cx - 10; x < cx + 10; x++) {
      putchar(is_slime_chunk(seed, x, z) ? '#' : ' ');
    }
    putchar('\n');
  }
}

void check_seed (uint64_t seed, uint32_t range, uint32_t size) {
  if (seed % LOG_INTERVAL == 0) {
    printf("%ld", -((int32_t)range));
    printf("%llu" LOG_SUFFIX "\n", seed / LOG_INTERVAL);
    show_area(seed, 0, 0);
  }
  for (int64_t x = -((int32_t)range); x < range; x++) {
    for (int64_t z = -((int32_t)range); z < range; z++) {
      if (!is_slime_chunk(seed, x, z)) continue;
      int area = 1;
      for (int32_t z_off = 0; z_off < size; z_off++) {
        for (int32_t x_off = 0; x_off < size; x_off++) {
          if (!is_slime_chunk(seed, x + x_off, z + z_off)) {
            area = 0;
            break;
          }
        }
        if (!area) break;
      }
      if (area) {
        printf("%lld (%lld, %lld) %lld,%lld\n", seed, x, z, x << 4, z << 4);
        show_area(seed, x, z);
      }
    }
  }
}

int main(int argc, char const *argv[]) {
  /*
  Random r = get_random(213478612348976ULL);
  for (int i = 0; i < 10; i++) printf("%x", random_next(&r, 32));
  putchar('\n');
  for (int i = 0; i < 1000; i++) printf("%u", random_next_int(&r, 10));
  putchar('\n');
  */
  uint64_t seed_start = 0;
  uint32_t range = 5;
  if (argc > 1) seed_start = atoll(argv[1]);
  if (argc > 2) range = atol(argv[2]);
  printf("Starting from %lld, range=%lu\n", seed_start, range);
  for (uint64_t seed = seed_start; seed != seed_start - 1; seed++) check_seed(seed, range, 3);
  return 0;
}
