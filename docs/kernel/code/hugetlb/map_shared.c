#include <assert.h>  // assert
#include <fcntl.h>   // open
#include <limits.h>  // INT_MAX
#include <math.h>    // sqrt
#include <stdbool.h> // bool false true
#include <stdio.h>
#include <stdlib.h> // malloc sort
#include <string.h> // strcmp ..
#include <sys/mman.h>
#include <unistd.h> // sleep

#define Mega (1 << 20)
#define TwoMega (2 * Mega)
int main(int argc, char *argv[]) {
  void *ptr = mmap(NULL, 8 * TwoMega, PROT_READ | PROT_WRITE,
                   MAP_SHARED | MAP_ANONYMOUS | MAP_HUGETLB, -1, 0);

  if (ptr != NULL) {
    printf("success !\n");
  } else {
    printf("failed !\n");
  }
  char *a = (char *)ptr;
  *a = '0';

  return 0;
}
