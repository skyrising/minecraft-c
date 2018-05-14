#include <jrand.cl>

__kernel void main(__global ulong *seeds, __global uchar *ret) {
  size_t id = get_global_id(0);
  /*
  ulong r = get_random(seeds[id]);
  ret[id] = (uchar) random_next_int(&r, 10);
  */
  ret[id] = (uchar) get_global_id(0);
}
