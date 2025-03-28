#include <ctype.h>
#include <errno.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define STRING_IMPL
#include "../../std.h/include/string.h"

#include "../../std.h/include/hash_map.h"

#include "neurolib.h"

typedef enum {
  JSON_NULL,
  JSON_STRING,
  JSON_NUMBER,
  JSON_BOOL,
  JSON_ARRAY,
  JSON_OBJECT
} JsonType_t;

typedef union JsonValue JsonValue_t;
typedef struct TaggedJsonValue TaggedJsonValue_t;

DA_DECLARE(TaggedJsonValue_t)
HM_DECLARE(String_t, TaggedJsonValue_t)

// an object is just a hashmap of elements
typedef HashMap_t(String_t, TaggedJsonValue_t) JsonObject_t;
// an array is just a dynamic array of elements
typedef DynamicArray_t(TaggedJsonValue_t) JsonArray_t;

union JsonValue {
  String_t string;
  double number;
  bool boolean;
  JsonArray_t array;
  JsonObject_t object;
};

struct TaggedJsonValue {
  JsonValue_t el;
  JsonType_t type;
};

HM_IMPL(String_t, TaggedJsonValue_t)
DA_IMPL(TaggedJsonValue_t)

// djb2 hashing algorithm
uint64_t s_hash(String_t *s) {
  uint64_t hash = 5381;
  Str_t s_slice = s_str(s);
  int c;

  while ((c = ss_advance_once(&s_slice)))
    hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

  return hash;
}

int s_eq(String_t *a, String_t *b) {
  if (a->len != b->len)
    return 0;

  for (size_t i = 0; i < a->len; i++)
    if (a->buf[i] != b->buf[i])
      return 0;

  return 1;
}

void json_deinit(TaggedJsonValue_t value);

void json_object_kv_deinit(KVPair_t(String_t, TaggedJsonValue_t) entry) {
  s_deinit(&entry.k);
  json_deinit(entry.v);
}

void json_deinit(TaggedJsonValue_t value) {
  switch (value.type) {
  case JSON_NULL:
  case JSON_BOOL:
  case JSON_NUMBER:
    break;
  case JSON_STRING:
    s_deinit(&value.el.string);
    break;
  case JSON_ARRAY:
    da_deinit(TaggedJsonValue_t)(&value.el.array, json_deinit);
    break;
  case JSON_OBJECT:
    hm_deinit(String_t, TaggedJsonValue_t)(&value.el.object,
                                           json_object_kv_deinit);
    break;
  }
}

int handle_escape_sequence(String_t *result, Str_t *s) {
  char unicode_buf[3] = {0};
  int escaped_char = ss_advance_once(s);
  if (!escaped_char)
    return 0;

  switch (escaped_char) {
  case '"':
  case '\\':
  case '/':
    return s_push(result, escaped_char);
  case 'b':
    return s_push(result, '\b');
  case 'f':
    return s_push(result, '\f');
  case 'n':
    return s_push(result, '\n');
  case 'r':
    return s_push(result, '\r');
  case 't':
    return s_push(result, '\t');
  case 'u':
    for (int i = 0; i < 2; i++) {
      // read two characters
      for (int j = 0; j < 2; j++) {
        int next_char = ss_advance_once(s);
        if (!next_char)
          return 0;

        // if any of the digits isn't a hex digit, we fail
        if (!isxdigit(next_char))
          return 0;

        unicode_buf[j] = next_char;
      }

      unicode_buf[2] = '\0';

      char *end;
      errno = 0;
      int res = strtoul(unicode_buf, &end, 16);

      if (end == unicode_buf || errno)
        return 0;

      if (!s_push(result, res))
        return 0;
    }
    return 1;
  default:
    return 0;
  }
}

int json_parse_string(Str_t *s, String_t *result) {
  *s = ss_trim_left(*s);

  if (s->len == 0 || *s->s != '"')
    return 0;

  // TODO: maybe handle this using an enum to differentiate between errors
  if (!s_init(result, 16))
    return 0;

  // skip the " character
  // won't fail
  ss_advance_once(s);
  while (s->len > 0) {
    // won't fail becasue len is at least 1
    int next_char = ss_advance_once(s);

    // filter control characters
    if (next_char >= 0 && next_char < ' ')
      break;

    switch (next_char) {
      // end of string
    case '"':
      return 1;
    case '\\':
      // if we can't properly handle the escape sequence, we have a skill issue
      // and fail
      if (!handle_escape_sequence(result, s)) {
        s_deinit(result);
        return 0;
      }
      break;
    default:
      // otherwise just append to the end of the string
      if (!s_push(result, next_char)) {
        s_deinit(result);
        return 0;
      }
    }
  }

  s_deinit(result);
  return 0;
}

// s needs to be null terminated for this function to work properly
int json_parse_number(Str_t *s, double *res) {
  char *end;
  errno = 0;
  // notice we can safely pass s->s into strtod because it is null terminated
  *res = strtod(s->s, &end);
  if (end == s->s || errno)
    return 0;

  size_t advance_bytes = end - s->s;
  ss_advance(s, advance_bytes);

  return 1;
}

int json_parse_boolean(Str_t *s, bool *result) {
  *s = ss_trim_left(*s);
  Str_t true_string = ss_from_cstring("true");
  Str_t false_string = ss_from_cstring("false");

  if (ss_starts_with(*s, true_string)) {
    *result = true;
    ss_advance(s, true_string.len);
    return 1;
  } else if (ss_starts_with(*s, false_string)) {
    *result = false;
    ss_advance(s, false_string.len);
    return 1;
  } else {
    return 0;
  }
}

int json_parse_null(Str_t *s) {
  *s = ss_trim_left(*s);
  Str_t null = ss_from_cstring("null");
  if (!ss_starts_with(*s, null))
    return 0;

  ss_advance(s, null.len);
  return 1;
}

int json_parse_value(Str_t *s, TaggedJsonValue_t *value);

int json_parse_array(Str_t *s, JsonArray_t *result) {
  *s = ss_trim_left(*s);
  if (s->len == 0 || *s->s != '[')
    return 0;

  if (!da_init(TaggedJsonValue_t)(result, 8))
    return 0;

  ss_advance_once(s);
  while (s->len > 0) {
    *s = ss_trim_left(*s);
    // end of array
    if (*s->s == ']') {
      if (!ss_advance_once(s))
        break;

      // only successfull case
      return 1;
    }

    TaggedJsonValue_t new_value;
    if (!json_parse_value(s, &new_value))
      break;

    if (!da_push(TaggedJsonValue_t)(result, new_value))
      break;

    *s = ss_trim_left(*s);

    // we need to support dangling commas and this code does just that. Notice
    // that this is *NOT* standard JSON
    if (*s->s == ',') {
      // trash the comma
      if (!ss_advance_once(s))
        break;
      // otherwise, if the next character is not a closing bracket, we have
      // failed
    } else if (*s->s != ']') {
      break;
    }
  }

  da_deinit(TaggedJsonValue_t)(result, json_deinit);
  return 0;
}

int json_parse_object(Str_t *s, JsonObject_t *result) {
  *s = ss_trim_left(*s);
  if (s->len == 0 || *s->s != '{')
    return 0;

  if (!hm_init(String_t, TaggedJsonValue_t)(result, 8, s_hash, s_eq))
    return 0;

  ss_advance_once(s);
  while (s->len > 0) {
    *s = ss_trim_left(*s);
    // end of array
    if (*s->s == '}') {
      ss_advance_once(s);
      // only successful case
      return 1;
    }

    String_t key;
    if (!json_parse_string(s, &key))
      break;

    *s = ss_trim_left(*s);
    if (ss_advance_once(s) != ':') {
      s_deinit(&key);
      break;
    }

    *s = ss_trim_left(*s);

    TaggedJsonValue_t new_value;
    if (!json_parse_value(s, &new_value)) {
      s_deinit(&key);
      break;
    }

    if (!hm_put(String_t, TaggedJsonValue_t)(result, key, new_value)) {
      s_deinit(&key);
      break;
    }

    *s = ss_trim_left(*s);

    // we need to support dangling commas and this code does just that. Notice
    // that this is *NOT* standard JSON
    if (*s->s == ',') {
      // trash the comma
      if (!ss_advance_once(s))
        break;
      // otherwise, if the next character is not a closing bracket, we have
      // failed
    } else if (*s->s != '}') {
      break;
    }
  }

  hm_deinit(String_t, TaggedJsonValue_t)(result, json_object_kv_deinit);
  return 0;
}

int json_parse_value(Str_t *s, TaggedJsonValue_t *value) {
  memset(value, 0, sizeof(TaggedJsonValue_t));

  *s = ss_trim(*s);

  if (json_parse_string(s, &value->el.string)) {
    value->type = JSON_STRING;
    return 1;
  } else if (json_parse_number(s, &value->el.number)) {
    value->type = JSON_NUMBER;
    return 1;
  } else if (json_parse_boolean(s, &value->el.boolean)) {
    value->type = JSON_BOOL;
    return 1;
  } else if (json_parse_null(s)) {
    value->type = JSON_NULL;
    return 1;
  } else if (json_parse_array(s, &value->el.array)) {
    value->type = JSON_ARRAY;
    return 1;
  } else if (json_parse_object(s, &value->el.object)) {
    value->type = JSON_OBJECT;
    return 1;
  } else {
    return 0;
  }
}

int json_parse(Str_t s, TaggedJsonValue_t *value) {
  s = ss_trim(s);
  if (!json_parse_value(&s, value))
    return 0;

  s = ss_trim(s);
  return s.len == 0;
}

#ifndef JSON_TESTS
static int print_usage(const char *prog) {
  fprintf(stderr, "Usage: %s [--extract <filename> | --bot]\n", prog);
  return 1;
}

// parse the content of the file given by filename into value
// Upon success, the caller is responsible for deiniting value
static int parse_json_file(const char *filename, TaggedJsonValue_t *value) {
  FILE *file = fopen(filename, "r");
  if (!file) {
    perror("could not open extraction file");
    return 0;
  }

  if (fseek(file, 0, SEEK_END) != 0) {
    if (fclose(file) != 0)
      perror("could not close extraction file");
    return 0;
  }

  long file_sz = ftell(file);
  rewind(file);

  // one extra for null
  char *buf = calloc(file_sz + 1, sizeof(char));
  if (!buf) {
    if (fclose(file) != 0)
      perror("could not close extraction file");

    return 0;
  }

  long items_read = fread(buf, file_sz, 1, file);
  if (items_read != 1) {
    if (ferror(file))
      perror("error while reading input file");
    else if (feof(file))
      fprintf(stderr,
              "could not read whole file because eof was encountered\n");

    if (fclose(file) != 0)
      perror("could not close extraction file");

    printf("huh?");
    free(buf);
    return 0;
  }

  if (fclose(file) != 0) {
    perror("could not close extraction file");
    free(buf);
    return 0;
  }

  Str_t json_data = ss_from_cstring(buf);

  if (!json_parse(json_data, value)) {
    fprintf(stderr, "Not an accepted JSON!\n");
    free(buf);
    return 0;
  }

  // this also nukes the json_data buffer
  free(buf);
  (void)json_data;
  return 1;
}

// this returns into res_s a pointer tied to the value owned by json. Therefore,
// if json is deinit'ed the res_s string slice is invalid
static int extract_content_field(TaggedJsonValue_t *json, Str_t *res_s) {
  // top level value must be an object
  if (json->type != JSON_OBJECT)
    return 0;

  String_t key;
  if (!s_init(&key, 10))
    return 0;

  if (!s_push_cstr(&key, "choices")) {
    s_deinit(&key);
    return 0;
  }

  // that contains a choices array
  TaggedJsonValue_t *choices =
      hm_get(String_t, TaggedJsonValue_t)(&json->el.object, &key);
  if (!choices || choices->type != JSON_ARRAY) {
    s_deinit(&key);
    return 0;
  }

  // with at least one element
  TaggedJsonValue_t *choices_0 =
      da_get(TaggedJsonValue_t)(&choices->el.array, 0);

  s_clear(&key);
  // which in turn contains a message object
  if (!choices_0 || !s_push_cstr(&key, "message")) {
    s_deinit(&key);
    return 0;
  }

  TaggedJsonValue_t *message =
      hm_get(String_t, TaggedJsonValue_t)(&choices_0->el.object, &key);

  s_clear(&key);
  // which finally has a content string in it
  if (!message || !s_push_cstr(&key, "content")) {
    s_deinit(&key);
    return 0;
  }

  TaggedJsonValue_t *content =
      hm_get(String_t, TaggedJsonValue_t)(&message->el.object, &key);
  if (!content || content->type != JSON_STRING) {
    s_deinit(&key);
    return 0;
  }

  s_deinit(&key);

  // the lifetime of res_s is tied to content and since content is part of the
  // json res_s is actually tied to the json
  *res_s = s_str(&content->el.string);

  return 1;
}

static int read_stdin_line(String_t *line) {
  s_clear(line);

  int c;
  // push characters until eof is found
  while ((c = getchar()) != EOF && c != '\n')
    if (!s_push(line, c))
      return 0;

  // we found eof, we need to notice the caller
  if (c == EOF)
    return EOF;

  // if we found a newline, we can just say we succeded
  return 1;
}

static int handle_api_response(const char *resp) {
  // read resp as a null-terminated string slice
  Str_t resp_as_str = ss_from_cstring(resp);

  TaggedJsonValue_t json;
  if (!json_parse(resp_as_str, &json)) {
    fprintf(stderr, "invalid json\n");
    return 0;
  }

  Str_t result;
  // the result of extract content field is tied to the lifetime of json
  if (!extract_content_field(&json, &result)) {
    json_deinit(json);
    fprintf(stderr, "could not locate target field in json\n");
    return 0;
  }

  // print message
  ss_print(stdout, result);
  printf("\n");
  fflush(stdout);

  // cleanup json
  json_deinit(json);

  return 1;
}

static int converse() {
  neurosym_init();

  String_t s;
  if (!s_init(&s, 16))
    return 1;

  int last_res;
  printf("> What would you like to know? ");
  while ((last_res = read_stdin_line(&s)) == 1) {
    // assume s needs to be null terminated (C moment)
    if (!s_push(&s, '\0')) {
      last_res = 0;
      break;
    }

    char *resp = response(s.buf);
    if (!resp || !handle_api_response(resp)) {
      last_res = 0;
      break;
    }

    // response requires we free the buffer
    free(resp);
    printf("> What would you like to know? ");
  }

  printf("Terminating\n");
  s_deinit(&s);
  return last_res != EOF;
}

static int extract(const char *filename) {
  TaggedJsonValue_t value;

  if (!parse_json_file(filename, &value))
    return 1;

  Str_t content_data;
  if (!extract_content_field(&value, &content_data)) {
    json_deinit(value);
    return 1;
  }

  for (size_t i = 0; i < content_data.len; i++)
    putchar(content_data.s[i]);

  printf("\n");
  fflush(stdout);

  json_deinit(value);
  return 0;
}

int main(int argc, const char **argv) {
  if (argc < 2 || argc > 3)
    return print_usage(argv[0]);

  if (strcmp(argv[1], "--extract") == 0) {
    if (argc != 3)
      return print_usage(argv[0]);

    return extract(argv[2]);
  } else if (strcmp(argv[1], "--bot") == 0) {
    if (argc != 2)
      return print_usage(argv[0]);

    return converse();
  } else {
    fprintf(stderr, "invalid option %s\n", argv[1]);
    return 1;
  }
}
#endif
