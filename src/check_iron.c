#include <stdio.h>
#include <inttypes.h>
#include "random.h"

static inline int check(uint64_t seed) {
  Random r = (Random) seed;
  int count = 0;
  while(random_next_int(&r, 7000) == 0) {
    random_next_int(&r, 16);
    random_next_int(&r, 6);
    random_next_int(&r, 16);
    random_next_int(&r, 50);
    count++;
  }
  return count;
}

void check_around(uint64_t seed) {
  uint64_t base = (seed >> 16) << 16;
  for (uint64_t i = 0; i < 1 << 16; i++) {
    int count = check(base + i);
    if (count == 3) {
      if (base + i == seed) printf("%lx is correct\n", seed);
      else printf("%lx was missing\n", base + i);
    } else if (base + i == seed) {
      printf("%lx is incorrect\n", seed);
    }
  }
}

int main(int argc, char **argv) {
  if (argc < 2) {
    fprintf(stderr, "Need file argument.");
    return -1;
  }
  FILE *f = fopen(argv[1], "r");
  if (!f) {
    perror("Cannot open file");
    return -2;
  }
  char line[100];
  fgets(line, 100, f); // discard first line
  
  uint64_t seed;
  while (!feof(f)) {
    if(!fscanf(f, "%lx", &seed)) break;
    check_around(seed);
  }
  return 0;
}
