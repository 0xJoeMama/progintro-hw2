# jason
A [JSON](https://json.org) parser written in C, with chat-bot conversation capabilities(for some reason).

# Building
To build the project you need GNU Make, a C compiler and an implementation of libc available on your system.

To build using GNU Make and gcc use the following:
```sh
$ make
```

To build using a different compiler, you can use:
```sh
$ make CC=yourcompiler
```

To change the flags passed to the compiler, you need to specify a CFLAGS section, as shown below:
```sh
$ make CFLAGS="-Wall -Wextra -fno-frame-pointer" # I don't know why you would want this but anyways
```

*NOTE: to delete build artifacts, you need to use the `clean` rule as shown below:*
```sh
$ make clean
```

# Running
After building a `jason` executable should be available in the top level directory.
If that is not the case, please leave an issue in the [issue tracker](https://github.com/progintro/hw2-0xJoeMama/issues).

To run the program just type the name of the executable followed by your prefered flags.
You can either extract the `json.choices[0].message.content` field from a file saved on the disc(with `--extract`) or enter conversation mode using `--bot`.
If you have an `OPENAI_API_KEY` environment variable set, it will be picked up and used an an OpenAI token in order to start conversation with a real LLM.
```sh
$ ./jason
Usage: ./jason [--extract <filename> | --bot]

$ ./jason --extract ./test.json
That’s classified information, but I trust you enough to pretend I don’t know either.

$ ./jason --bot
> What would you like to know? What time is it?
I’m 99% sure I’m 100% unsure about that.
> What would you like to know? Terminating

$ export OPENAI_API_KEY="<insert random string of characters that agrees with OpenAI's backend>"

$ ./jason --bot
> What would you like to know? What time is it?
I'm unable to check the current time. You can easily find out the time by checking a clock, your phone, or your computer.  # very helpful
> What would you like to know? Did you know that you are currently being prompted from a terminal?
I don't have awareness or perception of my environment, so I can't know how I'm being accessed. 
However, I can respond to prompts regardless of the interface! If you have questions or need assistance, feel free to ask.
```

As you can see, it easily prompts the AI overlords for a response and prints it for you in your fancy terminal.

# The Solution
## User Input
We first use the argument index 1 to find the mode we are operating in. If we enter extraction mode, we read another required argument.
Otherwise, we print a helpful error message.

## Type-System
Let's be honest, we really are all here about the actual JSON parser, not the extraction/conversation modes.
So first, the types. When I started, my brain screamed at me that this is the perfect usecase for a union. And I quite agree.
Because however, different types of JSON values require different handling, we have to use a tagged union, aka a union accompanied by a type enum.
Since we can use [std.h](https://github.com/0xJoeMama/std.h/), we can implement a JSON array as just an array of JSON values.
We can also implement JSON objects, as hash maps from strings to JSON values. That's how the following came to be:

```c
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
```

I had to fight the compiler quite a bit to get the above working, but the result is an amazingly beautiful set of types that looks very clean when it's used.

## Parsing
To parse JSON values, I used the format given by [the json.org website](https://json.org).
The parser, can handle all valid JSON files, regardless of the features used in them(unless they are out of JSON spec, more on that in a moment).

It boils down to the following functions which basically implement the basic building blocks of JSON, using serial parsing.
```c
int json_parse_string(Str_t *s, String_t *result);
int json_parse_number(Str_t *s, double *result);
int json_parse_boolean(Str_t *s, bool *result);
int json_parse_null(Str_t *s);
int json_parse_object(Str_t *s, JsonObject_t *result);
int json_parse_array(Str_t *s, JsonArray_t *result);
int json_parse_value(Str_t *s, TaggedJsonValue_t *value);
```
JSON is what is called a recursive structure so it can easily be parsed as so:
- The `json_parse_value` function calls all the other functions and succeeds when it finds at least one successful function.
- The `json_parse_array` function recursively calls into `json_parse_value`.
- The same is also true about the `json_parse_object` function.

The implementations are pretty simple(in fact it tooks me about 2 hours to get all of the functions working).
Notice that our parser does not support standard JSON. It allows for trailing commas.
The most interesting part of the parsing was definitely getting to see how useful the [std.h library](https://github.com/0xJoeMama/std.h/) is.

Also note that the [tests](./tests) folder, has a lot of unit tests for this whole ordeal.

Finally, to uninitialize JSON objects(which are heap allocated), we abuse the destructor system offered by [std.h](https://github.com/0xJoeMama/std.h/), by recursively calling into `json_deinit` function.

# The Future
I plan on including the JSON parser made in this exercise, as part of the std.h library. Wait until after the deadline(Feb 14th) and I will move it there.
