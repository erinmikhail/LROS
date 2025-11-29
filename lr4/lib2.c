#include "contracts.h"
#include <stdlib.h>
#include <string.h>

int prime_count(int a, int b) {
  if (b < 2)
    return 0;

  char *is_prime = malloc((b + 1) * sizeof(char));

  memset(is_prime, 1, b + 1);

  is_prime[0] = 0;
  is_prime[1] = 0;

  for (int p = 2; p * p <= b; p++) {
    if (is_prime[p]) {
      for (int i = p * p; i <= b; i += p)
        is_prime[i] = 0;
    }
  }

  int count = 0;
  int start = (a < 0) ? 0 : a;

  for (int i = start; i <= b; i++) {
    if (is_prime[i]) {
      count++;
    }
  }

  free(is_prime);
  return count;
}

void quicksort_impl(int *arr, int low, int high) {
  if (low >= high)
    return;

  int pivot = arr[(low + high) / 2];
  int i = low;
  int j = high;

  while (i <= j) {
    while (arr[i] < pivot)
      i++;
    while (arr[j] > pivot)
      j--;

    if (i <= j) {
      int temp = arr[i];
      arr[i] = arr[j];
      arr[j] = temp;
      i++;
      j--;
    }
  }

  if (low < j)
    quicksort_impl(arr, low, j);
  if (i < high)
    quicksort_impl(arr, i, high);
}

int *sort(int *array, size_t n) {
  if (n > 1) {
    quicksort_impl(array, 0, (int)n - 1);
  }
  return array;
}
