/*
 * partial_ownership.c: Demonstrate partial data ownership.
 *
 * Each thread owns its own counter (write-only), but can read other
 * threads' counters. This is a common pattern in statistical counters.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define NR_THREADS 4
#define CACHE_LINE_SIZE 64
#define ITERATIONS 1000000

struct per_thread_counter {
	unsigned long counter __attribute__((__aligned__(CACHE_LINE_SIZE)));
};

static struct per_thread_counter counters[NR_THREADS];
static pthread_t threads[NR_THREADS];

static void *thread_func(void *arg)
{
	long tid = (long)arg;
	unsigned long i;

	for (i = 0; i < ITERATIONS; i++)
		counters[tid].counter++;

	return NULL;
}

static unsigned long read_count(void)
{
	unsigned long sum = 0;
	int t;

	for (t = 0; t < NR_THREADS; t++)
		sum += counters[t].counter;

	return sum;
}

int main(void)
{
	long t;
	int rc;
	void *res;

	for (t = 0; t < NR_THREADS; t++) {
		rc = pthread_create(&threads[t], NULL, thread_func, (void *)t);
		if (rc != 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}

	for (t = 0; t < NR_THREADS; t++) {
		rc = pthread_join(threads[t], &res);
		if (rc != 0) {
			perror("pthread_join");
			exit(EXIT_FAILURE);
		}
	}

	printf("expected total: %lu\n", (unsigned long)NR_THREADS * ITERATIONS);
	printf("read_count:     %lu\n", read_count());
	return 0;
}
