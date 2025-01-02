#include <stddef.h>
#include <stdio.h>

double average_error(double data[], size_t data_sz, double delta) {
  double err_sum = 0;
  for (size_t i = 0; i < data_sz - 1; i++) {
    double calculated_y = delta + data[i];
    err_sum += data[i + 1] - calculated_y;
  }

  return err_sum / (data_sz - 1);
}

int main(void) {
  double data[] = {9, 7, 7, 1, 4, 4, 4, 38, 8, 4};
  size_t data_sz = sizeof(data) / sizeof(double);

  double slope = (data[data_sz - 1] - data[0]) / (data_sz - 1);
  double error = average_error(data, data_sz, slope);
  double estimate = slope + data[data_sz - 1] + error;

  printf("%.2f\n", estimate);

  return 0;
}
