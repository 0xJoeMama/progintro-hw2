#ifndef BITSET_H
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  uint64_t *buf;
  size_t len;
  size_t buf_sz;
} BitSet_t;

int bs_init(BitSet_t *bs, size_t elements);
void bs_deinit(BitSet_t *bs);

#define bs_is_set(bs, i) ((((bs)->buf[i >> 6] >> (i & 0x3F))) & 1)
#define bs_set(bs, i) ((bs)->buf[i >> 6] |= 1 << (i & 0x3F))
#define bs_reset(bs, i) ((bs)->buf[i >> 6] &= ~(1 << (i & 0x3F)))
#define bs_toggle(bs, i) ((bs)->buf[i >> 6] ^= 1 << (i & 0x3F))

#ifdef BITSET_IMPL
int bs_init(BitSet_t *bs, size_t elements) {
  size_t segments = elements >> 6;
  if (elements & 0x3F)
    segments++;

  uint64_t *buf = (uint64_t *)calloc(segments, sizeof(uint64_t));
  if (!buf)
    return 0;

  bs->len = elements;
  bs->buf_sz = segments;
  bs->buf = buf;

  return 1;
}

void bs_deinit(BitSet_t *bs) {
  free(bs->buf);
  memset(bs, 0, sizeof(BitSet_t));
}
#endif // BITSET_IMPL

#define BITSET_H
#endif // BITSET_H

