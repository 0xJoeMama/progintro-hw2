#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int print_usage(const char *prog_name) {
  fprintf(stderr, "Usage: %s <filename> [--window N (default: 50)]\n",
          prog_name);
  return 1;
}

int main(int argc, const char **argv) {
  if (argc < 2 || argc > 4)
    return print_usage(argv[0]);

  const char *filename = NULL;
  unsigned long long window_sz = 50;
  for (int i = 1; i < argc; i++) {
    if (strncmp("--window", argv[i], 8) == 0) {
      if (i + 1 >= argc)
        return print_usage(argv[0]);

      char *end;
      window_sz = strtoull(argv[i + 1], &end, 10);

      if (end == argv[i + 1])
        return print_usage(argv[0]);

      if (errno != 0) {
        perror("could not parse window size");
        return 1;
      }
      i++;
    } else {
      filename = argv[i];
    }
  }

  if (!filename)
    return print_usage(argv[0]);

  if (window_sz <= 0) {
    fprintf(stderr, "Window too small!\n");
    return 1;
  }

  double *window = calloc(window_sz, sizeof(double));
  if (!window) {
    fprintf(stderr, "Failed to allocate window memory\n");
    return 1;
  }

  FILE *value_file = fopen(filename, "r");
  if (!value_file) {
    perror("could not open value file");
    free(window);
    return 1;
  }

  size_t nums_read = 0;
  while (fscanf(value_file, "%lf", &window[nums_read % window_sz]) == 1)
    nums_read++;

  if (fclose(value_file) != 0) {
    perror("could not close value file");
    return 1;
  }

  if ((size_t)window_sz > nums_read) {
    fprintf(stderr, "Window too large!\n");
    free(window);
    return 1;
  }

  double sum = 0;
  for (unsigned long long i = 0; i < window_sz; i++)
    sum += window[i];

  double average = sum / window_sz;

  printf("%.2f\n", average);

  free(window);

  return 0;
}
