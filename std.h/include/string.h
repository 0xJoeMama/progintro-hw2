#ifndef STRING_H
#include "dynamic_array.h"

#ifdef STRING_IMPL
#define SS_IMPL
#include "string_slice.h"

DA_DECLARE_IMPL(char)
typedef DynamicArray_t(char) String_t;

int s_init(String_t *s, size_t initial_cap) {
  return da_init(char)(s, initial_cap);
}

int s_push(String_t *s, const char c) { return da_push(char)(s, c); }

int s_push_str(String_t *s, Str_t src) {
  for (const char *c = src.s; c < src.s + src.len; c++)
    if (!s_push(s, *c))
      return 0;

  return 1;
}

int s_push_cstr(String_t *s, const char *src) {
  Str_t str_src = ss_from_cstring(src);
  return s_push_str(s, str_src);
}

Str_t s_str(String_t *s) {
  return (Str_t){
      .s = s->buf,
      .len = s->len,
  };
}

void s_deinit(String_t *s) { da_deinit_char(s, NULL); }

void s_clear(String_t *s) {
  // no need to call da_clear bc chars can't possible have a destructor
  s->len = 0;
}
#else
#include "string_slice.h"

DA_DECLARE(char)
typedef DynamicArray_t(char) String_t;

int s_init(String_t *s, size_t initial_cap);
int s_push(String_t *s, const char c);
void s_clear(String_t *s);
int s_push_str(String_t *s, Str_t src);
int s_push_cstr(String_t *s, const char *src);
Str_t s_str(String_t *s);
void s_deinit(String_t *s);

#endif
#define STRING_H
#endif
