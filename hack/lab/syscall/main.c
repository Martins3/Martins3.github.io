// x86-64 Linux
#include <asm/unistd.h> // compile without -m32 for 64 bit call numbers
#include <sys/types.h>
#include <stdio.h>
// #define __NR_write 1

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void print_maps() {
  FILE *map;
  map = fopen("/proc/self/maps", "r");

  char line[512];

  while (!feof(map)) {
    if (fgets(line, 512, map) == NULL)
      break;
    printf("%s", line);
  }
}

ssize_t my_write(int fd, const void *buf, size_t size) {
  print_maps();

  ssize_t ret;
  asm volatile("syscall"
               : "=a"(ret)
               //                 EDI      RSI       RDX
               : "0"(__NR_write), "D"(fd), "S"(buf), "d"(size)
               : "rcx", "r11", "memory");
  return ret;
}

int main(int argc, char *argv[]) {
  size_t ret = my_write(1, "12345", 3);
  printf("\n--->%ld", ret);
  return 0;
}
