#include <stdio.h>
#include <stddef.h>
#include <string.h>

#ifndef STRING_SLICE_H

typedef struct {
  const char *s;
  size_t len;
} Str_t;

Str_t ss_from_cstring(const char *s);
int ss_cmp(Str_t a, Str_t b);
int ss_eq(Str_t a, Str_t b);

Str_t ss_trim_left(Str_t s);
Str_t ss_trim_right(Str_t s);
Str_t ss_trim(Str_t s);

Str_t ss_subslice(Str_t s, size_t start_inc, size_t end_exc);
Str_t ss_split_once(Str_t *s, char c);

int ss_advance_once(Str_t *s);
Str_t ss_advance(Str_t *s, size_t adv);
int ss_starts_with(Str_t s1, Str_t s2);
void ss_print(FILE* file, Str_t s);

#ifdef SS_IMPL
#include <ctype.h>

Str_t ss_from_cstring(const char *s) {
  size_t len = strlen(s);

  return (Str_t){
      .s = s,
      .len = len,
  };
}

int ss_cmp(Str_t a, Str_t b) {
  // first check all characters within the common size
  for (size_t i = 0; i < a.len && i < b.len; i++) {
    int diff = a.s[i] - b.s[i];
    if (diff != 0)
      return diff;
  }

  // if up to this point they are equal, this means their difference it just
  // their difference in size
  if (a.len < b.len) {
    return -1;
  } else if (a.len > b.len) {
    return 1;
  } else {
    return 0;
  }
}

Str_t ss_trim_left(Str_t s) {
  while (s.len > 0 && isspace(*s.s)) {
    s.s++;
    s.len--;
  }

  return s;
}

int ss_eq(Str_t a, Str_t b) {
  if (a.len != b.len)
    return 0;

  for (size_t i = 0; i < a.len; i++)
    if (a.s[i] != b.s[i])
      return 0;

  return 1;
}

Str_t ss_trim_right(Str_t s) {
  while (s.len > 0 && isspace(s.s[s.len - 1]))
    s.len--;

  return s;
}

Str_t ss_trim(Str_t s) {
  s = ss_trim_left(s);
  s = ss_trim_right(s);

  return s;
}

Str_t ss_subslice(Str_t s, size_t start_inc, size_t end_exc) {
  if (end_exc > s.len)
    end_exc = s.len;

  if (start_inc >= end_exc)
    return (Str_t){
        .len = 0,
        .s = s.s,
    };

  return (Str_t){
      .len = end_exc - 1 - start_inc,
      .s = s.s + start_inc,
  };
}

Str_t ss_split_once(Str_t *s, char c) {
  Str_t res = {
      .s = s->s,
      .len = 0,
  };

  while (*s->s != c && s->len > 0) {
    s->s++;
    s->len--;
    res.len++;
  }

  // if we didn't run out of characters, increment once to leave the c character
  // out of the result
  if (s->len > 0) {
    s->len--;
    s->s++;
  }

  return res;
}

int ss_advance_once(Str_t *s) {
  if (s->len == 0)
    return -1;

  s->len--;
  return *s->s++;
}

int ss_starts_with(Str_t s1, Str_t s2) {
  if (s1.len < s2.len)
    return 0;

  while (s2.len > 0)
    if (ss_advance_once(&s2) != ss_advance_once(&s1))
      return 0;

  return 1;
}

Str_t ss_advance(Str_t *s, size_t adv) {
  if (s->len < adv)
    return (Str_t){.len = 0, .s = NULL};

  Str_t res = {.len = adv, .s = s->s};
  s->s += adv;
  s->len -= adv;

  return res;
}

void ss_print(FILE* stream, Str_t s) {
  while (s.len > 0) {
    fputc(*s.s, stream);
    s.s++;
    s.len--;
  }
}
#endif // IMPL_SS

#define STRING_SLICE_H
#endif // STRING_SLICE_H
