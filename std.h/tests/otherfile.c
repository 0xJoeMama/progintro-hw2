#include "../include/hash_map.h"

HM_DECLARE(char, int)

int foo(void) {
  HashMap_t(char, int) map;
  // temporary names bc macros take too long to write
  if (!hm_init_char_int(&map, 10, NULL, NULL))
    return 1;

  hm_deinit_char_int(&map, NULL);

  return 0;
}
