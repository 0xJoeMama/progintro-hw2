#include "../include/hash_map.h"
#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int foo(void);

HM_DECLARE_IMPL(char, int)

uint64_t hash_char(char *c) { return *c; }
int char_eq(char *a, char *b) { return *a == *b; }

HM_DECLARE_IMPL(char, HashMap_t(char, int))

int main(void) {
  HashMap_t(char, int) map;
  // temporary names bc macros take too long to write
  if (!hm_init(char, int)(&map, 10, hash_char, char_eq))
    return 1;

  for (int i = 0; i < 256; i++) {
    if (!hm_put(char, int)(&map, i, i)) {
      hm_deinit(char, int)(&map, NULL);
      return 1;
    }
  }
  for (int i = 0; i < 256; i++) {
    char c = i;
    printf("%c -> %d\n", i, *hm_get_char_int(&map, &c));
    int c_p;
    if (!hm_remove(char, int)(&map, &c, &c_p)) {
      hm_deinit(char, int)(&map, NULL);
      return 1;
    }

    assert(hm_get(char, int)(&map, &c) == NULL && "woopsie");
  }

  printf("cap = %zu, len = %zu\n", map.cap, map.len);
  hm_put(char, int)(&map, 'a', 32);
  char c = 'a';
  *hm_get(char, int)(&map, &c) += 10;
  printf("%c -> %d\n", c, *hm_get(char, int)(&map, &c));

  hm_deinit(char, int)(&map, NULL);
  foo();

  return 0;
}
