// x86-64 Linux
#include <asm/unistd.h> // compile without -m32 for 64 bit call numbers
#include <sys/types.h>
#include <stdio.h>
// #define __NR_write 1

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ssize_t my_write(int fd, const void *buf, size_t size) {
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
  printf("return is %ld", ret);
  return 0;
}
