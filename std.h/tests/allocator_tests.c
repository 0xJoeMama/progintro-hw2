#include <assert.h>
#define ALLOC_IMPL
#include "../include/allocator.h"

struct StackStruct {
  int x, y;
  float z;
};

int main(void) {
  Mallocator_t mallocator = mallocator_get();

  ArenaAllocator_t arena;
  if (!arena_init(&arena, 1024, &mallocator)) return 1;

  struct StackStruct *test = calloc(&arena, struct StackStruct, 1);
  if (!test)
    return 1;

  test->x = 1;
  test->y = 1;
  test->z = 1.02;

  test = realloc(&arena, test, 6 * sizeof(struct StackStruct));

  void *v = alloc(&mallocator, 12);
  if (!v)
    return 1;

  v = realloc(&mallocator, v, 24);
  if (!v) {
    dealloc(&mallocator, v);
    arena_deinit(&arena);
    return 1;
  }

  dealloc(&mallocator, v);

  arena_deinit(&arena);

  return 0;
}
