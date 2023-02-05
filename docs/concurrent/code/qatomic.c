#include <assert.h> // assert
#include <errno.h>  // strerror
#include <fcntl.h>  // open
#include <limits.h> // INT_MAX
#include <math.h>   // sqrt
#include <pthread.h>
#include <stdbool.h> // bool false true
#include <stdio.h>
#include <stdlib.h> // malloc sort
#include <stdlib.h>
#include <string.h> // strcmp
#include <string.h>
#include <unistd.h> // sleep
#include <unistd.h>

#define qatomic_set__nocheck(ptr, i) __atomic_store_n(ptr, i, __ATOMIC_RELAXED)

#define qatomic_set(ptr, i)                                                    \
  do {                                                                         \
    qatomic_set__nocheck(ptr, i);                                              \
  } while (0)

#define qatomic_add(ptr, n) ((void)__atomic_fetch_add(ptr, n, __ATOMIC_SEQ_CST))

pthread_t tid[2];
int counter;

void *trythis(void *arg) {
  unsigned long i = 10000000;
  printf("[huxueshi:%s:%d] \n", __FUNCTION__, __LINE__);
  while (i--) {
    qatomic_set(&counter, counter + 1);
    // qatomic_add(&counter, 1);
  }
  return NULL;
}

int main(int argc, char *argv[]) {
  int i = 0;
  int error;

  while (i < 2) {
    error = pthread_create(&(tid[i]), NULL, &trythis, NULL);
    if (error != 0)
      printf("\nThread can't be created : [%s]", strerror(error));
    i++;
  }

  pthread_join(tid[0], NULL);
  pthread_join(tid[1], NULL);
  printf("[huxueshi:%s:%d] %d\n", __FUNCTION__, __LINE__, counter);

  return 0;
}
