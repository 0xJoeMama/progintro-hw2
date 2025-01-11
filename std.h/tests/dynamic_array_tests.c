#include "../include/dynamic_array.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

DA_DECLARE_IMPL(int)
DA_DECLARE_IMPL(char)

typedef struct as {
  int x;
} as_t;

typedef struct {
  int x;
  char **y;
  char name[200];
} Asd_t;

// array
DA_DECLARE_IMPL(Asd_t)
// array of arrays
DA_DECLARE_IMPL(DynamicArray_t(Asd_t))
// array of arrays of arrays
DA_DECLARE_IMPL(DynamicArray_t(DynamicArray_t(Asd_t)))

void asd_t_deinit(Asd_t asd) { free(asd.y); }

int main(void) {
  DynamicArray_t(int) da;
  if (!da_init(int)(&da, 5))
    return 1;

  for (int i = 0; i < 100; i++)
    da_push(int)(&da, i);

  assert(da.len == 100 && "bad array length");
  assert(da.cap == 160 && "bad array length");

  int curr;
  int end = 99;
  while (da_pop(int)(&da, &curr))
    assert(curr == end-- && "bad popped value");

  assert(da.len == 0 && "bad length after popping");
  assert(da.cap == 1 && "bad capacity after and resizing");

  da_deinit(int)(&da, NULL);

  DynamicArray_t(Asd_t) new_arr;

  if (!da_init(Asd_t)(&new_arr, 10))
    return 1;

  Asd_t v = {.x = 12,
             .y = malloc(sizeof(char *)),
             .name = "Asdasdasdasdasdasdasdasdassssssssssssssssss"};
  da_push(Asd_t)(&new_arr, v);

  for (size_t i = 0; i < new_arr.len; i++)
    printf("%s\n", da_get(Asd_t)(&new_arr, i)->name);

  da_deinit(Asd_t)(&new_arr, asd_t_deinit);

  // Hey! Vsauce. Michael here.
  // *This* is a string. Or is it?
  DynamicArray_t(char) is_this_a_string;
  if (!da_init(char)(&is_this_a_string, 10))
    return 1;

  char asd[] = "asdasdasdasdasd";
  for (size_t i = 0; i < sizeof(asd) / sizeof(char); i++)
    da_push(char)(&is_this_a_string, asd[i]);

  da_clear(char)(&is_this_a_string, NULL);
  assert(is_this_a_string.len == 0 && "bad clear function");
  da_deinit(char)(&is_this_a_string, NULL);

  return 0;
}
