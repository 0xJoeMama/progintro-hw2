#include <assert.h>
#include <stdio.h>

#define SS_IMPL
#include "../include/string_slice.h"

void print_ss(Str_t s) {
  for (size_t i = 0; i < s.len; i++)
    putchar(s.s[i]);
}

int main(void) {
  Str_t s1 = ss_from_cstring("Hello world");
  Str_t s2 = ss_from_cstring("hello world");
  Str_t s3 = ss_from_cstring("");
  assert(ss_cmp(s1, s2) < 0 && "invalid comparison");
  assert(ss_cmp(s1, s3) > 0 && "comparing with empty string failed");

  Str_t spaces = ss_from_cstring("                            aaa");
  spaces = ss_trim_left(spaces);
  Str_t aaa = ss_from_cstring("aaa");

  assert(ss_eq(spaces, aaa) && "left trim does not work");

  Str_t f2 = ss_from_cstring("        42\n\n\n\n\v\t\r     ");
  Str_t f2_new = ss_trim_right(f2);

  Str_t right_strip = ss_from_cstring("        42");
  assert(ss_eq(right_strip, f2_new) && "right trim does not work");

  Str_t f2_good = ss_from_cstring("42");
  f2 = ss_trim(f2);
  assert(ss_eq(f2, f2_good) && "could not equate trimmed string");

  Str_t hello = ss_subslice(s2, 0, 6);
  Str_t hello_good = ss_from_cstring("hello");
  assert(ss_eq(hello, hello_good) && "subslicing is broken");

  Str_t huh = ss_subslice(hello, 1, 0);
  assert(ss_eq(huh, s3) && "problematic edge case");

  Str_t hello_world = s2;
  Str_t hello_new = ss_split_once(&hello_world, ' ');
  assert(ss_eq(hello_new, hello_good) && "splitting does not work");

  return 0;
}
