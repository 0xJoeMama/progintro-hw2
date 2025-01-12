#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../std.h/include/dynamic_array.h"

#define SS_IMPL
#include "../../std.h/include/string_slice.h"

// a line in the input file
typedef struct {
  uint8_t cities[2];
  int32_t cost;
} CityEntry_t;

typedef int32_t **DistanceMatrix_t;

// versions of dynamic array that are used
DA_DECLARE_IMPL(Str_t)
DA_DECLARE_IMPL(int64_t)
DA_DECLARE_IMPL(DynamicArray_t(int64_t))
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

// read the contents of input into a dynamically allocated buffer and return the
// result in out_buf
static int read_input(FILE *input, Str_t *out_s) {
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

  out_s->s = data;
  out_s->len = file_sz;

  return 1;
}

static int find_city(Str_t niddle, DynamicArray_t(Str_t) * haystack) {
  for (size_t i = 0; i < haystack->len; i++)
    if (ss_eq(haystack->buf[i], niddle))
      return (int)i;

  return -1;
}

static int read_city_until(Str_t *line, char delim,
                           DynamicArray_t(Str_t) * cities) {
  Str_t city = ss_trim(ss_split_once(line, delim));
  int idx = find_city(city, cities);
  if (idx < 0) {
    // cant fail since it's stack allocated
    da_push(Str_t)(cities, city);
    idx = cities->len - 1;
  }

  return idx;
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
    line = ss_trim(ss_split_once(&buf, '\n'));
    int loc_1 = read_city_until(&line, '-', cities);
    int loc_2 = read_city_until(&line, ':', cities);

    memset(local_buf, 0, sizeof(local_buf));
    if (line.len > sizeof(local_buf)) {
      fprintf(stderr, "number exceeded 2^31\n");
      return 0;
    }

    memcpy(local_buf, line.s, line.len);

    CityEntry_t entry = {.cities = {loc_1, loc_2},
                         .cost = (int32_t)strtol(local_buf, &end, 10)};

    // WARN: errno code does not compile with -m32
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

  // create distance matrix
  DistanceMatrix_t costs = (DistanceMatrix_t)create_heap_table(
      cities->len, cities->len, sizeof(int32_t));

  if (!costs) {
    da_deinit(CityEntry_t)(&distances, NULL);
    return 0;
  }

  for (CityEntry_t *curr = distances.buf; curr < distances.buf + distances.len;
       curr++) {
    // we are doing this symmetrically
    // technically speaking we could save on some memory but it's infinite
    // anyways so who cares(:upside_down:)
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

  memo->city_cnt = cities->len;

  return 1;
}

static void free_memo(Memo_t *memo) {
  free_heap_table(memo->city_cnt, (void **)memo->dists);
  memset(memo, 0, sizeof(Memo_t));
}

// check if the bit-th bit is set on the bitset bs
#define is_set(bs, bit) (bs & (1 << bit))
static void int64_arr_destroy(DynamicArray_t(int64_t) arr) {
  da_deinit(int64_t)(&arr, NULL);
}

typedef DynamicArray_t(DynamicArray_t(int64_t)) CombinationBuffer_t;

// fill combs with arrays that contain all i-element subsets of a set of n
// elements
// The combs array must be uninitialized, will be fill with the results and must
// be deinited by the caller upon success
static int generate_combination_matrix(CombinationBuffer_t *combs, int n) {
  if (!da_init(DynamicArray_t(int64_t))(combs, n + 1))
    return 0;

  int64_t nchoosek = 1;
  for (int k = 0; k <= n; k++) {
    DynamicArray_t(int64_t) current;
    if (!da_init(int64_t)(&current, nchoosek)) {
      da_deinit(DynamicArray_t(int64_t))(combs, int64_arr_destroy);
      return 0;
    }

    if (!da_push(DynamicArray_t(int64_t))(combs, current)) {
      int64_arr_destroy(current);
      da_deinit(DynamicArray_t(int64_t))(combs, int64_arr_destroy);
      return 0;
    }

    nchoosek = (nchoosek * (n - k)) / (k + 1);
  }

  for (int64_t i = 0; i < (1 << n); i++) {
    int ones = __builtin_popcount(i);

    DynamicArray_t(int64_t) *bucket = &combs->buf[ones];

    if (!da_push(int64_t)(bucket, i)) {
      da_deinit(DynamicArray_t(int64_t))(combs, int64_arr_destroy);
      return 0;
    }
  }

  return 1;
}

// populate memo with the solutions to tsp using the cost as the
// adjacency matrix
// Implementation of the pseudocode given here:
// https://web.archive.org/web/20150208031521/http://www.cs.upc.edu/~mjserna/docencia/algofib/P07/dynprog.pdf
// memo should be an initialized Memo_t object and cost should be an initialized
// adjacency matrix
int held_karp_tsp(DistanceMatrix_t cost, Memo_t *memo) {
  // initialize 2 element subsets
  for (int i = 1; i < memo->city_cnt; i++)
    memo->dists[i][(1 | (1 << i))] = cost[0][i];

  CombinationBuffer_t combs;
  if (!generate_combination_matrix(&combs, memo->city_cnt))
    return 0;

  // for all subsets with 3 or more elements
  for (int s = 3; s <= memo->city_cnt; s++) {
    DynamicArray_t(int64_t) *k_el_subsets = &combs.buf[s];
    for (size_t S_idx = 0; S_idx < k_el_subsets->len; S_idx++) {
      int64_t S = k_el_subsets->buf[S_idx];
      // if the last city is set(meaning it's the beginning), skip it
      if ((S & 1) == 0)
        continue;

      // otherwise
      // for all cities
      for (register int k = 0; k < memo->city_cnt; k++) {
        // toggle the next-th bit of subset aka remove next from the subset
        int64_t S_prime = S ^ (1 << k);
        // find minimum
        // inf placeholder
        int64_t min = INT64_MAX;
        for (register int m = 1; m < memo->city_cnt; m++) {
          if (!is_set(S, m))
            continue;

          // separating these conditions allows for optimizations by the
          // compiler source? perf
          if (!(m ^ k))
            continue;

          int64_t new_dist = memo->dists[m][S_prime] + cost[m][k];

          if (new_dist < min)
            min = new_dist;
        }

        // cache the result
        memo->dists[k][S] = min;
      }
    }
  }

  // delete the combinations array
  da_deinit(DynamicArray_t(int64_t))(&combs, int64_arr_destroy);

  return 1;
}

// given a populated memo object, use it to reconstruct the optimal tsp tour,
// into the tour array
// The tour array must be given uninitialized, will be populated with the
// results and must be deinited upon success by the caller.
static int construct_tour(Memo_t *memo, DistanceMatrix_t costs,
                          DynamicArray_t(int) * tour) {
  int last_idx = -1;
  int64_t state = (1 << memo->city_cnt) - 1;

  if (!da_init(int)(tour, memo->city_cnt))
    return 0;

  // start from the last city
  for (int i = memo->city_cnt - 1; i >= 1; i--) {
    int idx = 1;
    // start at first non-zero bit after the 0th bit
    while (!is_set(state, idx))
      idx++;

    // find the index that minimizes it's distance(our path is the minimum so we
    // always minimize distance)
    for (int j = idx; j < memo->city_cnt; j++) {
      if (!is_set(state, j))
        continue;

      int64_t prev, new;
      if (last_idx < 0) {
        // only on the first run, we find the total minimum
        prev = memo->dists[idx][state];
        new = memo->dists[j][state];
      } else {
        // otherwise we find the minimum when compared to the old value
        prev = memo->dists[idx][state] + costs[idx][last_idx];
        new = memo->dists[j][state] + costs[j][last_idx];
      }

      if (new < prev)
        idx = j;
    }

    // add found city to tour
    if (!da_push(int)(tour, idx)) {
      da_deinit(int)(tour, NULL);
      return 0;
    }

    // update the current subset
    state ^= (1 << idx);
    last_idx = idx;
  }

  if (!da_push(int)(tour, 0)) {
    da_deinit(int)(tour, NULL);
    return 0;
  }

  return 1;
}

static void print_results(DynamicArray_t(int) * route,
                          DynamicArray_t(Str_t) * cities,
                          DistanceMatrix_t costs) {
  printf("We will visit cities in the following order:\n");
  // total cost accumulator
  int64_t cost = 0;
  for (int i = (int)route->len - 1; i >= 0; i--) {
    int curr_idx = route->buf[i];
    ss_print(stdout, cities->buf[curr_idx]);
    // for all elements except the last one
    if (i == 0)
      continue;

    // distance to previous element
    int32_t curr_cost = costs[curr_idx][route->buf[i - 1]];
    // print its distance to it's previous element
    printf(" -(%" PRId32 ")-> ", curr_cost);
    // and add it to the accumulator
    cost += curr_cost;
  }
  printf("\n");

  printf("Total cost: %" PRId64 "\n", cost);
}

int main(int argc, const char **argv) {
  if (argc != 2)
    return print_usage(argv[0]);

  FILE *input_map = fopen(argv[1], "r");
  if (!input_map) {
    perror("could not open input file");
    return 1;
  }

  Str_t file_data = {0};
  int read_res = read_input(input_map, &file_data);

  if (fclose(input_map) != 0) {
    perror("could not close input file");
    // if it's null nothing happens according to man page
    free((void *)file_data.s);
    return 1;
  }

  if (!read_res) {
    free((char *)file_data.s);
    return 1;
  }

  // we will always have less than or equal to 64 cities so we are safe to do
  // this
  Str_t cities_buf[64];
  DistanceMatrix_t costs = NULL;
  DynamicArray_t(Str_t) cities = {.buf = cities_buf, .cap = 64, .len = 0};
  if (!parse_input(file_data, &cities, &costs)) {
    // cleanup
    free((char *)file_data.s);
    return 1;
  }

  if (cities.len == 0) {
    fprintf(stderr, "input file does not contain any cities\n");
    return 1;
  }

  Memo_t memo;
  if (!create_memo(&cities, &memo)) {
    // cleanup
    free_heap_table(cities.len, (void **)costs);
    free((char *)file_data.s);
    return 1;
  }

  // solve the problem using the Held-Karp algorithm for TSP
  if (!held_karp_tsp(costs, &memo)) {
    // cleanup
    free_heap_table(cities.len, (void **)costs);
    free((char *)file_data.s);
    free_memo(&memo);
    return 1;
  }

  DynamicArray_t(int) route;
  if (!construct_tour(&memo, costs, &route)) {
    // cleanup
    free_heap_table(cities.len, (void **)costs);
    free_memo(&memo);
    free((char *)file_data.s);
    return 1;
  }

  print_results(&route, &cities, costs);

  // cleanup
  da_deinit(int)(&route, NULL);
  free_heap_table(cities.len, (void **)costs);
  free_memo(&memo);
  free((char *)file_data.s);

  return 0;
}
