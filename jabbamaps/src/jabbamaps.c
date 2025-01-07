#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../std.h/include/dynamic_array.h"

#define SS_IMPL
#include "../../std.h/include/string_slice.h"

typedef struct {
  uint8_t cities[2];
  int32_t cost;
} CityEntry_t;

typedef int32_t **DistanceMatrix_t;

DA_DECLARE_IMPL(Str_t)
DA_DECLARE_IMPL(int64_t)
DA_DECLARE_IMPL(int)
DA_DECLARE_IMPL(CityEntry_t)

static int print_usage(const char *prog) {
  fprintf(stderr, "Usage: %s <filename>\n", prog);
  return 1;
}

static void **create_heap_table(size_t rows, size_t cols, size_t el_sz) {
  void **dists = calloc(rows, sizeof(void *));
  if (!dists)
    return NULL;

  // since the zero-th city is never used we need to reduce the amount of
  // entries by 1 in ever set
  for (size_t i = 0; i < rows; i++) {
    dists[i] = calloc(cols, el_sz);
    if (!dists[i]) {
      for (size_t j = 0; j < i; j++)
        free(dists[j]);

      free(dists);
      return NULL;
    }
  }

  return dists;
}

static void free_heap_table(size_t rows, void **buf) {
  for (size_t i = 0; i < rows; i++)
    free(buf[i]);

  free(buf);
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

static int parse_input(Str_t buf, DynamicArray_t(Str_t) * cities,
                       DistanceMatrix_t *costs_out) {
  char local_buf[12];
  char *end;
  Str_t line;

  DynamicArray_t(CityEntry_t) distances;
  if (!da_init(CityEntry_t)(&distances, 16))
    return 0;

  buf = ss_trim(buf);
  while (buf.len != 0) {
    line = ss_split_once(&buf, '\n');
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
      return 0;
    }

    memcpy(local_buf, line.s, line.len);

    CityEntry_t entry;

    entry.cities[0] = loc_1;
    entry.cities[1] = loc_2;

    entry.cost = (int32_t)strtol(local_buf, &end, 10);

    // WARN: the following code does not compile with -m32 because we are
    // missing some headers for errno
    // assuming we cannot fail with ERANGE
    if (end == local_buf /*|| errno != 0*/) {
      fprintf(stderr, "could not read integer from buffer %12s\n", local_buf);
      da_deinit(CityEntry_t)(&distances, NULL);
      return 0;
    }

    if (!da_push(CityEntry_t)(&distances, entry)) {
      da_deinit(CityEntry_t)(&distances, NULL);
      return 0;
    }
  }

  int32_t **costs =
      (int32_t **)create_heap_table(cities->len, cities->len, sizeof(int32_t));

  if (!costs) {
    da_deinit(CityEntry_t)(&distances, NULL);
    return 0;
  }

  for (CityEntry_t *curr = distances.buf; curr < distances.buf + distances.len;
       curr++) {
    costs[curr->cities[0]][curr->cities[1]] = curr->cost;
    costs[curr->cities[1]][curr->cities[0]] = curr->cost;
  }

  *costs_out = costs;

  da_deinit(CityEntry_t)(&distances, NULL);
  return 1;
}

typedef struct {
  int64_t **dists;
  int city_cnt;
} Memo_t;

static int create_memo(DynamicArray_t(Str_t) * cities, Memo_t *memo) {
  size_t subsets = 1 << cities->len;
  memo->dists =
      (int64_t **)create_heap_table(cities->len, subsets, sizeof(int64_t));
  if (!memo->dists)
    return 0;

  // initialize all cells to INT64_MIN
  for (size_t i = 0; i < cities->len; i++)
    memset(memo->dists[i], 0xFF, subsets * sizeof(int64_t));

  memo->city_cnt = cities->len;

  return 1;
}

static void free_memo(Memo_t *memo) {
  free_heap_table(memo->city_cnt, (void **)memo->dists);
  memset(memo, 0, sizeof(Memo_t));
}

#define is_set(bs, bit) (bs & (1 << bit))

int generate_combinations(DynamicArray_t(int64_t) * combs, int n, int k) {
  combs->len = 0;
  for (int64_t i = 0; i < (1 << n); i++)
    if (__builtin_popcount(i) == k)
      if (!da_push(int64_t)(combs, i))
        return 0;

  return 1;
}

int tsp(DistanceMatrix_t cost, Memo_t *memo) {
  // initialize 2 element subsets
  for (int i = 1; i < memo->city_cnt; i++)
    memo->dists[0][(1 | (1 << i))] = cost[0][i];

  DynamicArray_t(int64_t) combs;
  if (!da_init(int64_t)(&combs, 1 << memo->city_cnt))
    return -1;

  for (int i = 3; i <= memo->city_cnt; i++) {
    if (!generate_combinations(&combs, memo->city_cnt, i)) {
      fprintf(stderr, "failed to allocate memory for combinations buffer\n");
      da_deinit(int64_t)(&combs, NULL);
      return 0;
    }

    for (size_t j = 0; j < combs.len; j++) {
      int64_t subset = combs.buf[j];
      if ((subset & 1) == 0)
        continue;

      for (int next_node = 0; next_node < memo->city_cnt; next_node++) {
        // toggle the next-th bit of subset aka remove next from the subset
        int64_t state = subset ^ (1 << next_node);
        // inf placeholder
        int64_t min = INT64_MAX;
        for (int end_node = 1; end_node < memo->city_cnt; end_node++) {
          if (end_node == next_node || !is_set(subset, end_node))
            continue;

          int64_t new_dist =
              memo->dists[end_node][state] + cost[end_node][next_node];

          if (new_dist < min)
            min = new_dist;
        }

        memo->dists[next_node][subset] = min;
      }
    }
  }

  da_deinit(int64_t)(&combs, NULL);

  return 1;
}

int construct_tour(Memo_t *memo, DistanceMatrix_t costs,
                   DynamicArray_t(int) * tour) {
  size_t last_idx = 0;
  uint64_t state = (1 << memo->city_cnt) - 1;
  if (!da_init(int)(tour, 64))
    return 0;

  int idx = -1;
  for (int i = (int)memo->city_cnt - 1; i >= 0; i--) {
    for (int j = 1; j < memo->city_cnt; j++) {
      if (idx < 0)
        idx = j;
      int64_t prev_dist = memo->dists[idx][state];
      int64_t new_dist = memo->dists[j][state] + costs[j][last_idx];

      if (new_dist < prev_dist)
        idx = j;
    }

    if (!da_push(int)(tour, i)) {
      da_deinit(int)(tour, NULL);
      return 0;
    }
    state = state ^ (1 << idx);
    last_idx = idx;
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
  DistanceMatrix_t costs = NULL;
  DynamicArray_t(Str_t) cities = {.buf = cities_buf, .cap = 64, .len = 0};

  if (!parse_input(file_data, &cities, &costs)) {
    free((char *)file_data.s);
    return 1;
  }

  Memo_t memo;
  if (!create_memo(&cities, &memo)) {
    free_heap_table(cities.len, (void **)costs);
    free((char *)file_data.s);
    return 1;
  }

  // TODO: handle error case
  tsp(costs, &memo);

  DynamicArray_t(int) route;
  if (!construct_tour(&memo, costs, &route))
    return 1;

  da_deinit(int)(&route, NULL);
  free_heap_table(cities.len, (void **)costs);
  free_memo(&memo);
  free((char *)file_data.s);

  return 0;
}
