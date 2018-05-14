#include <jrand.cl>

__kernel void main(__global ulong *seeds, __global uchar *ret) {
  size_t id = get_global_id(0);
  int max_count = 0;
  for (ulong i = 0; i < 1 << 16; i++) {
    int count = 0;
    ulong r = get_random((seeds[id] << 16) | i);
    while (random_next_int(&r, 7000) == 0) {
      random_next_int(&r, 16);
      random_next_int(&r, 6);
      random_next_int(&r, 16);
      random_next_int(&r, 50);
      count++;
    }
    if (count > max_count) max_count = count;
  }
  ret[id] = (uchar) max_count;
}
