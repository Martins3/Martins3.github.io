// 用于方便的制造出来 oom 的
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

unsigned long malloc_batch = (1 << 30);
unsigned int page_size = 4 << 10;

int main() {
  int i;
  unsigned long j;
  char *p;

  for (i = 0; i < 100; i++) {
    p = mmap(0, malloc_batch, PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, 0, 0);
    if (p == MAP_FAILED) {
      printf("malloc failed\n");
    }
    printf("allocated %d GB vm\n", i + 1);
    printf("starting allocated page\n");
    for (j = 0; j < malloc_batch; j += page_size) {
      *(p + j) = 'a';
    }

    printf("starting allocated page done, allocated %ld pages\n", j + 1);
  }
}
