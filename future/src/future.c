#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, const char **argv) {
  if (argc < 2 || argc > 4) {
    printf("Usage: %s <filename> [--window N (default: 50)]\n", argv[0]);
    return 1;
  }

  const char *filename = NULL;
  long long window_sz = 50;
  for (int i = 1; i < argc; i++) {
    if (strncmp("--window", argv[i], 8) == 0) {
      if (i + 1 >= argc) {
        fprintf(stderr, "expected an integer as window size");
        return 1;
      }

      window_sz = strtoull(argv[i + 1], NULL, 10);
      if (errno != 0) {
        perror("could not parse window size: ");
        return 1;
      }
      i++;
    } else {
      filename = argv[i];
    }
  }

  if (!filename) {
    fprintf(stderr, "filename was not provided\n");
    return 1;
  }

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
    perror("could not open value file: ");
    return 1;
  }

  long long nums_read = 0;
  while (fscanf(value_file, "%lf", &window[nums_read % window_sz]) == 1)
    nums_read++;

  // TODO: handle error case
  fclose(value_file);

  if (window_sz > nums_read) {
    fprintf(stderr, "Window too large!\n");
    free(window);
    return 1;
  }

  double sum = 0;
  for (long long i = 0; i < window_sz; i++)
    sum += window[i];

  double average = sum / window_sz;

  printf("%.2f\n", average);

  free(window);

  return 0;
}
