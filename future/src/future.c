#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *value_filepath;
  size_t window_sz;
} Config_t;

static int print_usage(const char *prog_name) {
  fprintf(stderr, "Usage: %s <filename> [--window N (default: 50)]\n",
          prog_name);
  return 1;
}

static int parse_cli(int argc, const char **argv, Config_t *cfg) {
  if (argc < 2 || argc > 4)
    return print_usage(argv[0]);

  cfg->window_sz = 50;
  cfg->value_filepath = NULL;

  for (int i = 1; i < argc; i++) {
    if (strncmp("--window", argv[i], 8) == 0) {
      // since we need to peek into the next argument, if it is not provided, we
      // need to fail
      if (i + 1 >= argc)
        return print_usage(argv[0]);

      char *end;
      // parse window size as an unsigned long long and cast to size_t
      errno = 0;
      cfg->window_sz = (size_t)strtoull(argv[i + 1], &end, 10);

      // if strtoull failed print usage and fail
      if (end == argv[i + 1] || errno != 0)
        return print_usage(argv[0]);

      // we need to increment i since we just consumed the integer argument
      // after it
      i++;
    } else if (!cfg->value_filepath) {
      // if filename is not initialized, we initialize it to the first
      // non-argument string
      cfg->value_filepath = argv[i];
    } else {
      // otherwise, we fail because an extra argument was provided
      return print_usage(argv[0]);
    }
  }

  // since filename is a required argument, we fail if we it does not have a
  // value
  if (!cfg->value_filepath)
    return print_usage(argv[0]);

  // check if window size is 0
  if (cfg->window_sz == 0) {
    fprintf(stderr, "Window too small!\n");
    return 1;
  }

  return 0;
}

int main(int argc, const char **argv) {
  Config_t cfg = {0};
  // parse the command line arguments
  if (parse_cli(argc, argv, &cfg) != 0)
    return 1;

  // allocate window
  double *window = calloc(cfg.window_sz, sizeof(double));
  if (!window) {
    fprintf(stderr, "Failed to allocate window memory\n");
    return 1;
  }

  // read values into window
  FILE *value_file = fopen(cfg.value_filepath, "r");
  if (!value_file) {
    perror("could not open value file");
    free(window);
    return 1;
  }

  // keep track of the amount of numbers read
  size_t nums_read = 0;
  // keep track of first failing scanf result
  int res;
  while ((res = fscanf(value_file, "%lf",
                       &window[nums_read % cfg.window_sz])) == 1)
    nums_read++;

  if (res == 0) {
    fprintf(stderr, "could not parse number at index %zu\n", nums_read);
    return 1;
  }

  // if res isnt 0, it's definitely EOF, which means we finished reading the
  // file, so we can close it
  if (fclose(value_file) != 0) {
    perror("could not close value file");
    free(window);
    return 1;
  }

  // handle too large windows
  if (cfg.window_sz > nums_read) {
    fprintf(stderr, "Window too large!\n");
    free(window);
    return 1;
  }

  // calculate the average of the window array
  double sum = 0;
  for (size_t i = 0; i < cfg.window_sz; i++)
    sum += window[i];

  // division is safe, since window_sz == 0 has already been handled
  double average = sum / cfg.window_sz;

  printf("%.2lf\n", average);

  free(window);

  return 0;
}
