/*
 * read_mostly_demo.c
 * Demonstrates the performance benefit of read-mostly data access patterns.
 *
 * perfbook Ch.3 "Hardware Optimizations" mentions read-mostly replication:
 * data that is frequently read but rarely updated can be replicated across
 * all CPUs' caches, allowing extremely fast access.
 *
 * This program compares:
 * 1. Read-only shared data (replicated in all caches)
 * 2. Read-write shared data (cache line bouncing)
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>

#define ARRAY_SIZE (1024 * 1024)
#define ITERATIONS 100000000ULL

static uint64_t *shared_array;
static uint64_t write_counter = 0;

static double get_time_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static void *thread_read_only(void *arg)
{
	int thread_id = *(int *)arg;
	uint64_t local_sum = 0;
	uint64_t start_idx = (ARRAY_SIZE / 4) * thread_id;
	uint64_t end_idx = start_idx + (ARRAY_SIZE / 4);

	for (uint64_t i = 0; i < ITERATIONS; i++) {
		/* Read from read-only shared data */
		local_sum += shared_array[start_idx + (i % (end_idx - start_idx))];
	}

	/* Prevent optimization */
	*(volatile uint64_t *)&local_sum;
	return NULL;
}

static void *thread_read_mostly(void *arg)
{
	(void)arg;
	uint64_t local_sum = 0;

	for (uint64_t i = 0; i < ITERATIONS; i++) {
		/* Mostly read, occasional write */
		if ((i & 0xFFFF) == 0) {
			__atomic_fetch_add(&write_counter, 1, __ATOMIC_SEQ_CST);
		}
		local_sum += shared_array[i % ARRAY_SIZE];
	}

	*(volatile uint64_t *)&local_sum;
	return NULL;
}

static void *thread_heavy_write(void *arg)
{
	(void)arg;
	uint64_t local_sum = 0;

	for (uint64_t i = 0; i < ITERATIONS; i++) {
		/* Frequent writes cause cache-line bouncing */
		shared_array[i % ARRAY_SIZE]++;
		local_sum += shared_array[i % ARRAY_SIZE];
	}

	*(volatile uint64_t *)&local_sum;
	return NULL;
}

static void run_read_only(int nthreads)
{
	pthread_t threads[4];
	int tids[4];

	/* Initialize read-only data */
	for (size_t i = 0; i < ARRAY_SIZE; i++) {
		shared_array[i] = i;
	}

	double start = get_time_ns();
	for (int i = 0; i < nthreads; i++) {
		tids[i] = i;
		pthread_create(&threads[i], NULL, thread_read_only, &tids[i]);
	}
	for (int i = 0; i < nthreads; i++) {
		pthread_join(threads[i], NULL);
	}
	double end = get_time_ns();

	printf("Read-only (%d threads):          %.3f ms\n",
	       nthreads, (end - start) / 1e6);
}

static void run_read_mostly(int nthreads)
{
	pthread_t threads[4];
	int tids[4];
	write_counter = 0;

	for (size_t i = 0; i < ARRAY_SIZE; i++) {
		shared_array[i] = i;
	}

	double start = get_time_ns();
	for (int i = 0; i < nthreads; i++) {
		tids[i] = i;
		pthread_create(&threads[i], NULL, thread_read_mostly, &tids[i]);
	}
	for (int i = 0; i < nthreads; i++) {
		pthread_join(threads[i], NULL);
	}
	double end = get_time_ns();

	printf("Read-mostly (%d threads):        %.3f ms\n",
	       nthreads, (end - start) / 1e6);
}

static void run_heavy_write(int nthreads)
{
	pthread_t threads[4];
	int tids[4];

	for (size_t i = 0; i < ARRAY_SIZE; i++) {
		shared_array[i] = i;
	}

	double start = get_time_ns();
	for (int i = 0; i < nthreads; i++) {
		tids[i] = i;
		pthread_create(&threads[i], NULL, thread_heavy_write, &tids[i]);
	}
	for (int i = 0; i < nthreads; i++) {
		pthread_join(threads[i], NULL);
	}
	double end = get_time_ns();

	printf("Heavy write (%d threads):        %.3f ms\n",
	       nthreads, (end - start) / 1e6);
}

int main(void)
{
	printf("=== Read-Mostly Demo ===\n");
	printf("Array size: %d elements, iterations: %llu\n\n",
	       ARRAY_SIZE, (unsigned long long)ITERATIONS);

	shared_array = calloc(ARRAY_SIZE, sizeof(uint64_t));
	if (!shared_array) {
		perror("calloc");
		return 1;
	}

	run_read_only(1);
	run_read_only(2);
	run_read_only(4);
	printf("\n");

	run_read_mostly(1);
	run_read_mostly(2);
	run_read_mostly(4);
	printf("\n");

	run_heavy_write(1);
	run_heavy_write(2);
	run_heavy_write(4);

	printf("\nExplanation:\n");
	printf("  Read-only data is replicated in all CPU caches.\n");
	printf("  Heavy write causes cache-line ownership to bounce between CPUs.\n");
	printf("  Read-mostly strikes a balance: reads are fast (cached),\n");
	printf("  while writes are rare enough to not dominate overhead.\n");

	free(shared_array);
	return 0;
}
