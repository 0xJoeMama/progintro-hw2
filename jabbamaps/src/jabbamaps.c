#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SS_IMPL
#include "../../std.h/include/dynamic_array.h"

#define SS_IMPL
#include "../../std.h/include/string_slice.h"

DA_IMPL(Str_t);
DA_IMPL(int);

static int print_usage(const char *prog) {
  fprintf(stderr, "Usage: %s <filename>\n", prog);
  return 1;
}

static int read_input(FILE *input, Str_t *out_buf) {
  // go to the end
  // returns non-zero on failure
  if (fseek(input, 0, SEEK_END) != 0) {
    perror("could not find the size of input file");
    return 0;
  }
  // get the current position
  long eof = ftell(input);
  // return to the beginning
  rewind(input);

  size_t file_sz = eof;

  char *data = (char *)malloc(file_sz * sizeof(char));
  if (!data) {
    perror("could not allocate memory");
    return 0;
  }

  size_t bytes_read = (size_t)fread(data, sizeof(char), file_sz, input);
  if (bytes_read != file_sz) {
    fprintf(stderr, "read %zu bytes instead of %zu\n", file_sz, bytes_read);
    return 0;
  }

  printf("Size of file is %zu, contents: %20s", file_sz, data);

  out_buf->s = data;
  out_buf->len = file_sz;

  return 1;
}

// TODO: move this into DA and change it's name to index of or get index
static int find_city(Str_t niddle, const DynamicArray_t(Str_t) * haystack) {
  for (size_t i = 0; i < haystack->len; i++)
    if (ss_eq(haystack->buf[i], niddle))
      return (int)i;

  return -1;
}

void ss_print(Str_t s) {
  while (s.len > 0) {
    putchar(*s.s);
    s.s++;
    s.len--;
  }
}


// TODO: create dynamic array set which will expand the array
static int parse_input(Str_t buf, DynamicArray_t(Str_t) * cities) {
  char local_buf[12];
  char *end;
  Str_t line;

  while ((line = ss_split_once(&buf, '\n')).len != 0) {
    Str_t city_1 = ss_trim(ss_split_once(&line, '-'));
    Str_t city_2 = ss_trim(ss_split_once(&line, ':'));

    int loc_1 = find_city(city_1, cities);
    if (loc_1 < 0) {
      da_push(Str_t)(cities, city_1);
      loc_1 = cities->len - 1;
    }

    int loc_2 = find_city(city_2, cities);
    if (loc_2 < 0) {
      da_push(Str_t)(cities, city_2);
      loc_2 = cities->len - 1;
    }

    memset(local_buf, 0, sizeof(local_buf));
    if (line.len > sizeof(local_buf)) {
      fprintf(stderr, "number exceeded 2^31\n");
      return 1;
    }

    memcpy(local_buf, line.s, line.len);

    errno = 0;
    int dist = strtol(local_buf, &end, 10);

    if (end == local_buf || errno != 0) {
      fprintf(stderr, "could not read integer from buffer %12s\n", local_buf);
      return 1;
    }

    printf("Distance between cities ");
    ss_print(city_1);
    printf(" and ");
    ss_print(city_2);
    printf(" is %d\n", dist);
  }

  return 1;
}

int main(int argc, const char **argv) {
  if (argc != 2)
    return print_usage(argv[0]);

  FILE *input_map = fopen(argv[1], "r");
  if (!input_map)
    return print_usage(argv[0]);

  Str_t file_data = {0};
  int read_res = read_input(input_map, &file_data);

  if (fclose(input_map) != 0) {
    perror("could not close input file");
    // if it's null nothing happens according to man page
    free((void *)file_data.s);
    return 1;
  }

  if (!read_res)
    return 1;

  // we will always have less than or equal to 64 cities so we are safe to do
  // this
  Str_t cities_buf[64];
  DynamicArray_t(Str_t) cities = {.buf = cities_buf, .cap = 64, .len = 0};

  if (!parse_input(file_data, &cities)) {
    free((char *)file_data.s);
    return 1;
  }

  return 0;
}
