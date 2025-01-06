#include <stdbool.h>
#include <string.h>

#define STRING_IMPL
#include "../../std.h/include/string.h"

#include "../../std.h/include/hash_map.h"

typedef enum {
  JSON_NULL,
  JSON_STRING,
  JSON_NUMBER,
  JSON_BOOL,
  JSON_ARRAY,
  JSON_OBJECT
} JsonType_t;

typedef union JsonElement JsonElement_t;
typedef struct TaggedJsonElement TaggedJsonElement_t;

DA_DECLARE(TaggedJsonElement_t)
HM_DECLARE(String_t, TaggedJsonElement_t)

// an object is just a hashmap of elements
typedef HashMap_t(String_t, TaggedJsonElement_t) JsonObject_t;
// an array is just a dynamic array of elements
typedef DynamicArray_t(TaggedJsonElement_t) JsonArray_t;

union JsonElement {
  String_t string;
  double number;
  bool boolean;
  JsonArray_t array;
  JsonObject_t object;
};

struct TaggedJsonElement {
  JsonElement_t el;
  JsonType_t type;
};

HM_IMPL(String_t, TaggedJsonElement_t)
DA_IMPL(TaggedJsonElement_t)

int parse_element(Str_t s, TaggedJsonElement_t *el) {
  memset(el, 0, sizeof(TaggedJsonElement_t));
  return 1;
}

int main() { return 0; }
