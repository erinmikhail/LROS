#include <dlfcn.h>
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
  void *handle;
  int (*prime_count)(int, int);
  int *(*sort)(int *, size_t);

  const char *libs[] = {"./lib1.so", "./lib2.so"};
  int cur_lib = 0;

  handle = dlopen(libs[cur_lib], RTLD_LAZY);
  if (!handle) {
    print_msg("Library open error\n");
    return 1;
  }

  prime_count = dlsym(handle, "prime_count");
  sort = dlsym(handle, "sort");

  char input[1024];
  char output[1024];

  snprintf(output, sizeof(output), "Program 2 (Dynamic). Lib: %s\n",
           libs[cur_lib]);
  print_msg(output);
  print_msg("0 to switch, 1 for primes, 2 for sort\n");

  int bytes_read;
  while ((bytes_read = read(0, input, sizeof(input) - 1)) > 0) {
    input[bytes_read] = '\0';

    int cmd, offset;
    if (sscanf(input, "%d%n", &cmd, &offset) != 1)
      continue;
    char *args = input + offset;

    if (cmd == 0) {
      dlclose(handle);
      cur_lib = 1 - cur_lib;
      handle = dlopen(libs[cur_lib], RTLD_LAZY);
      if (!handle) {
        print_msg("Error switching lib\n");
        return 1;
      }
      prime_count = dlsym(handle, "prime_count");
      sort = dlsym(handle, "sort");

      snprintf(output, sizeof(output), "Switched to %s\n", libs[cur_lib]);
      print_msg(output);
    } else if (cmd == 1) {
      int a, b;
      if (sscanf(args, "%d %d", &a, &b) == 2) {
        int res = prime_count(a, b);
        snprintf(output, sizeof(output), "Primes count: %d\n", res);
        print_msg(output);
      }
    } else if (cmd == 2) {
      int arr[MAX_ARR_SIZE];
      size_t n = 0;
      int val, read_chars;
      while (sscanf(args, "%d%n", &val, &read_chars) == 1) {
        if (n < MAX_ARR_SIZE)
          arr[n++] = val;
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
      }
    }
  }

  dlclose(handle);
  return 0;
}
