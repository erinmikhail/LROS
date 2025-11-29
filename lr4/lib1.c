#include "contracts.h"

int is_prime(int n) {
  if (n < 2)
    return 0;
  for (int i = 2; i < n; i++) {
    if (n % i == 0)
      return 0;
  }
  return 1;
}

int prime_count(int a, int b) {
  int count = 0;
  for (int i = a; i <= b; i++) {
    if (is_prime(i)) {
      count++;
    }
  }
  return count;
}

int *sort(int *array, size_t n) {
  if (n < 2)
    return array;

  for (size_t i = 0; i < n - 1; i++) {
    for (size_t j = 0; j < n - i - 1; j++) {
      if (array[j] > array[j + 1]) {
        int temp = array[j];
        array[j] = array[j + 1];
        array[j + 1] = temp;
      }
    }
  }
  return array;
}
