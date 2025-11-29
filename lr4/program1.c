#include "contracts.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAX_ARR_SIZE 100

void print_msg(const char *msg) {
  int len = 0;
  while (msg[len] != '\0')
    len++;
  write(1, msg, len);
}

int main() {
  char input[1024];
  char output[1024];

  print_msg("Program 1 (Static). Enter:\n");
  print_msg("1 <a> <b>       - Count primes in [a, b]\n");
  print_msg("2 <n1> <n2> ... - Sort array\n");

  int bytes_read;
  while ((bytes_read = read(0, input, sizeof(input) - 1)) > 0) {
    input[bytes_read] = '\0';

    int cmd;
    int offset;
    if (sscanf(input, "%d%n", &cmd, &offset) != 1) {
      print_msg("Invalid command\n");
      continue;
    }

    char *args = input + offset;

    if (cmd == 1) {
      int a, b;
      if (sscanf(args, "%d %d", &a, &b) == 2) {
        int res = prime_count(a, b);
        snprintf(output, sizeof(output), "Primes count: %d\n", res);
        print_msg(output);
      } else {
        print_msg("Error reading arguments for prime count\n");
      }
    } else if (cmd == 2) {
      int arr[MAX_ARR_SIZE];
      size_t n = 0;
      int val, read_chars;

      while (sscanf(args, "%d%n", &val, &read_chars) == 1) {
        if (n < MAX_ARR_SIZE) {
          arr[n++] = val;
        }
        args += read_chars;
      }

      if (n > 0) {
        sort(arr, n);
        print_msg("Sorted: ");
        for (size_t i = 0; i < n; i++) {
          snprintf(output, sizeof(output), "%d ", arr[i]);
          print_msg(output);
        }
        print_msg("\n");
      } else {
        print_msg("Empty array or parse error\n");
      }
    } else {
      print_msg("Unknown command\n");
    }
  }
  return 0;
}
