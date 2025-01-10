#include <assert.h>

#define STRING_IMPL
#include "../include/string.h"

int main() {
  String_t s;
  s_init(&s, 10);
  s_push_cstr(&s, "hello good world");
  s_push(&s, '!');
  s_push_str(&s, ss_from_cstring("Hello?"));
  Str_t expectation = ss_from_cstring("hello good world!Hello?");
  assert(ss_eq(expectation, s_str(&s)) && "string does not work :cryge:");
  s_deinit(&s);
  return 1;
}
