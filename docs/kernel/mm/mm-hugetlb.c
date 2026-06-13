#include <errno.h>
#include <stdio.h>
#include <stdlib.h> // malloc sort
#include <string.h> // strcmp ..
#include <sys/mman.h>
#include <unistd.h> // sleep

#define PG_SIZE (1 << 21)

int main(int argc, char *argv[]) {
  long num = 100;
  char *ptr = (char *)mmap(NULL, num * PG_SIZE, PROT_READ | PROT_WRITE,
                           MAP_ANONYMOUS | MAP_PRIVATE | MAP_HUGETLB, -1, 0);

  if (ptr == MAP_FAILED) {
    printf("error %s\n", strerror(errno));
    exit(1);
  }

  // for (unsigned long i = 0; i < num; ++i) {
  //   char *addr = ptr + i * (1 << 12);
  //   *addr = 'x';
  // }

  sleep(10000);

  return 0;
}
