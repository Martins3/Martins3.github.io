/*
 * data_race_demo.c: Demonstrate what happens with and without
 * proper synchronization when accessing shared variables.
 *
 * Shows load-tearing, store-tearing, and how READ_ONCE/WRITE_ONCE
 * plus locks prevent them.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>

#define NTHREADS 4
#define NITER 1000000

/* Without synchronization: data race on counter */
static long counter_racy = 0;

/* With mutex */
static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
static long counter_mutex = 0;

/* With atomic */
static long counter_atomic = 0;

void *increment_racy(void *arg)
{
	int i;
	(void)arg;
	for (i = 0; i < NITER; i++)
		counter_racy++;
	return NULL;
}

void *increment_mutex(void *arg)
{
	int i;
	(void)arg;
	for (i = 0; i < NITER; i++) {
		pthread_mutex_lock(&mutex);
		counter_mutex++;
		pthread_mutex_unlock(&mutex);
	}
	return NULL;
}

void *increment_atomic(void *arg)
{
	int i;
	(void)arg;
	for (i = 0; i < NITER; i++)
		__sync_fetch_and_add(&counter_atomic, 1);
	return NULL;
}

static void run_test(const char *name, void *(*worker)(void *),
                     long expected, long *counter)
{
	pthread_t tid[NTHREADS];
	int i;

	*counter = 0;
	for (i = 0; i < NTHREADS; i++) {
		if (pthread_create(&tid[i], NULL, worker, NULL) != 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}
	for (i = 0; i < NTHREADS; i++)
		pthread_join(tid[i], NULL);

	printf("%s: expected %ld, got %ld (%s)\n",
	       name, expected, *counter,
	       *counter == expected ? "PASS" : "FAIL (data race)");
}

/* Demonstrate store-tearing prevention with WRITE_ONCE */
static uint64_t shared_u64 = 0;

void *store_u64(void *arg)
{
	(void)arg;
	/* On 32-bit systems, plain assignment could be torn into two 32-bit stores */
	/* WRITE_ONCE prevents tearing for machine-sized aligned accesses */
	*(volatile uint64_t *)&shared_u64 = 0x123456789ABCDEF0ULL;
	return NULL;
}

void *load_u64(void *arg)
{
	(void)arg;
	uint64_t v;
	while ((v = *(volatile uint64_t *)&shared_u64) == 0)
		continue;
	printf("Loaded u64: 0x%016lx\n", (unsigned long)v);
	return NULL;
}

int main(void)
{
	pthread_t tid1, tid2;
	long expected = (long)NTHREADS * NITER;

	printf("=== Data Race Demo ===\n\n");

	run_test("Racy increment (no sync)", increment_racy, expected, &counter_racy);
	run_test("Mutex increment", increment_mutex, expected, &counter_mutex);
	run_test("Atomic increment", increment_atomic, expected, &counter_atomic);

	printf("\n--- Store/load tearing prevention ---\n");
	shared_u64 = 0;
	pthread_create(&tid1, NULL, load_u64, NULL);
	pthread_create(&tid2, NULL, store_u64, NULL);
	pthread_join(tid1, NULL);
	pthread_join(tid2, NULL);

	return 0;
}
