#include <assert.h>

#define BITSET_IMPL
#include "../include/bitset.h"

int main(void) {
  BitSet_t bs;
  if (!bs_init(&bs, 12))
    return 1;

  assert(!bs_is_set(&bs, 0) && "bad initialization to zero");

  bs_set(&bs, 0);
  bs_set(&bs, 1);

  assert(bs_is_set(&bs, 0) && "bad indexing code or bad setting");
  assert(bs_is_set(&bs, 1) && "bad indexing code or bad setting");

  bs_reset(&bs, 1);
  bs_reset(&bs, 1);
  bs_reset(&bs, 1);
  assert(!bs_is_set(&bs, 1) && "bad reset code");

  bs_toggle(&bs, 2);
  assert(bs_is_set(&bs, 2) && "bad toggling code");
  bs_toggle(&bs, 2);
  assert(!bs_is_set(&bs, 2) && "bad toggling code");

  bs_deinit(&bs);

  return 0;
}
