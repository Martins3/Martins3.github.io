/*
 * designated_thread.c: Demonstrate designated thread pattern.
 *
 * A special thread is assigned ownership of a specific function and
 * its related data. Here, the "eventual()" thread periodically pulls
 * per-thread counts into a global counter, providing eventual consistency.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>

#define NR_THREADS 4
#define CACHE_LINE_SIZE 64
#define ITERATIONS 500000

struct per_thread_counter {
	unsigned long counter __attribute__((__aligned__(CACHE_LINE_SIZE)));
};

static struct per_thread_counter counters[NR_THREADS];
static unsigned long global_count;
static atomic_int stopflag;
static pthread_t worker_threads[NR_THREADS];
static pthread_t designated_tid;

static void *eventual(void *arg)
{
	unsigned long sum;
	int t;
	(void)arg;

	while (atomic_load(&stopflag) < 3) {
		sum = 0;
		for (t = 0; t < NR_THREADS; t++)
			sum += counters[t].counter;
		global_count = sum;
		usleep(1000); /* 1ms polling interval */
		if (atomic_load(&stopflag))
			atomic_fetch_add(&stopflag, 1);
	}
	return NULL;
}

static void *worker_thread(void *arg)
{
	long tid = (long)arg;
	unsigned long i;

	for (i = 0; i < ITERATIONS; i++)
		counters[tid].counter++;

	return NULL;
}

int main(void)
{
	long t;
	int rc;
	void *res;

	rc = pthread_create(&designated_tid, NULL, eventual, NULL);
	if (rc != 0) {
		perror("pthread_create designated");
		exit(EXIT_FAILURE);
	}

	for (t = 0; t < NR_THREADS; t++) {
		rc = pthread_create(&worker_threads[t], NULL, worker_thread, (void *)t);
		if (rc != 0) {
			perror("pthread_create worker");
			exit(EXIT_FAILURE);
		}
	}

	for (t = 0; t < NR_THREADS; t++) {
		rc = pthread_join(worker_threads[t], &res);
		if (rc != 0) {
			perror("pthread_join worker");
			exit(EXIT_FAILURE);
		}
	}

	atomic_store(&stopflag, 1);
	while (atomic_load(&stopflag) < 3)
		usleep(1000);

	pthread_join(designated_tid, &res);

	printf("expected total: %lu\n", (unsigned long)NR_THREADS * ITERATIONS);
	printf("global_count:   %lu\n", global_count);
	return 0;
}
