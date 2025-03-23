#include <assert.h>

#define JSON_TESTS
#include "../src/jason.c"

void test_number() {
  TaggedJsonValue_t json;
  const char *json_data = "-123.22342348E-122";
  Str_t json_data_slice = ss_from_cstring(json_data);
  assert(json_parse(json_data_slice, &json) == 1 &&
         "could not parse json string");
  assert(json.type == JSON_NUMBER && "not a number ?!");
  double expect = -123.22342348E-122;
  assert(expect == json.el.number && "bad number");
  json_deinit(json);
}

void test_boolean() {
  TaggedJsonValue_t json;
  const char *json_data = "false";
  Str_t json_data_slice = ss_from_cstring(json_data);
  assert(json_parse(json_data_slice, &json) == 1 &&
         "could not parse json string");
  assert(json.type == JSON_BOOL && "not a bool ?!");
  assert(!json.el.boolean && "bad boolean");
  json_deinit(json);
}

void test_string() {
  TaggedJsonValue_t json;
  const char *json_data =
      "\"hello good world \\n \\r \\t              \\f\\\" \"";
  Str_t json_data_slice = ss_from_cstring(json_data);
  assert(json_parse(json_data_slice, &json) == 1 &&
         "could not parse json string");
  assert(json.type == JSON_STRING && "not a string ?!");
  Str_t expect = ss_from_cstring("hello good world \n \r \t              \f\" ");
  assert(ss_eq(expect, s_str(&json.el.string)) && "bad string");
  s_deinit(&json.el.string);
  json_deinit(json);
}

void test_null() {
  TaggedJsonValue_t json;
  const char *json_data = "null";
  Str_t json_data_slice = ss_from_cstring(json_data);
  assert(json_parse(json_data_slice, &json) == 1 &&
         "could not parse json string");
  assert(json.type == JSON_NULL && "not a null?!");
  json_deinit(json);
}

void test_array() {
  TaggedJsonValue_t json;
  const char *json_data =
      "    [  1, 3 , \"dsfsdfvsdf \\\" \", true, false ,null     ,] \n \n\t ";
  Str_t json_data_slice = ss_from_cstring(json_data);
  assert(json_parse(json_data_slice, &json) == 1 &&
         "could not parse json string");
  assert(json.type == JSON_ARRAY && "not a array?!");
  assert(json.el.array.len == 6 && "bad array length");
  json_deinit(json);
}

void test_object() {
  TaggedJsonValue_t json;
  const char *json_data = "{ \"1\": 1, \"2\": [1, 2, 3], \"3\": 123.3e-2, "
                          "\"4\": {}, \"5\": true, \"6\": null, }";
  Str_t json_data_slice = ss_from_cstring(json_data);
  assert(json_parse(json_data_slice, &json) == 1 &&
         "could not parse json string");
  assert(json.type == JSON_OBJECT && "not a object?!");
  assert(json.el.object.len == 6 && "bad array length");
  json_deinit(json);
}

void test_unicode() {
  TaggedJsonValue_t json;
  const char *json_data = "\"\\u00a3\"";
  Str_t json_data_slice = ss_from_cstring(json_data);
  assert(json_parse(json_data_slice, &json) == 1 &&
         "could not parse json string");
  assert(json.type == JSON_STRING && "not a string?!");
  assert(json.el.string.len == 2);
  assert(json.el.string.buf[0] == 0);
  assert(json.el.string.buf[1] == (char)0xA3);
  json_deinit(json);
}

int main(void) {
  test_string();
  test_number();
  test_boolean();
  test_null();
  test_array();
  test_object();
  test_unicode();
  return 0;
}
