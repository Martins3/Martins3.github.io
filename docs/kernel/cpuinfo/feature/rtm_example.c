#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define NUM_THREADS 10
#define NUM_ITERATIONS 100000

typedef struct {
  int counter;
} TMCounter;

#define USE_TM 1

#ifdef USE_TM
void tm_increment(TMCounter *tm_counter) {
  __transaction_atomic { tm_counter->counter++; }
}
#else
void tm_increment(TMCounter *tm_counter) { tm_counter->counter++; }
#endif

void *increment_counter(void *arg) {
  TMCounter *tm_counter = (TMCounter *)arg;
  for (int i = 0; i < NUM_ITERATIONS; i++) {
    tm_increment(tm_counter);
  }
  return NULL;
}

int main() {
  pthread_t threads[NUM_THREADS];
  TMCounter tm_counter;
  tm_counter.counter = 0;

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_create(&threads[i], NULL, increment_counter, &tm_counter);
  }

  for (int i = 0; i < NUM_THREADS; i++) {
    pthread_join(threads[i], NULL);
  }

  printf("Counter: %d\n", tm_counter.counter);

  return 0;
}
