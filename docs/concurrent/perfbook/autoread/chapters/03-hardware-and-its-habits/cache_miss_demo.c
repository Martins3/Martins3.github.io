/*
 * cache_miss_demo.c
 * Demonstrates the cost of cache misses in multi-threaded programs.
 *
 * This program compares three scenarios:
 * 1. Single thread incrementing a local counter (no cache miss)
 * 2. Two threads incrementing separate counters on different cache lines
 *    (no false sharing, no cache miss)
 * 3. Two threads incrementing the same shared counter (cache line bouncing)
 *
 * Scenario 3 corresponds to the "Cache Misses" section in perfbook Ch.3.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#define ITERATIONS 100000000ULL

static volatile uint64_t shared_counter = 0;
static uint64_t counter_a __attribute__((aligned(128))) = 0;
static uint64_t counter_b __attribute__((aligned(128))) = 0;

static double get_time_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static void *thread_local_increment(void *arg)
{
	volatile uint64_t *counter = arg;
	for (uint64_t i = 0; i < ITERATIONS; i++) {
		(*counter)++;
	}
	return NULL;
}

static void *thread_shared_increment(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < ITERATIONS; i++) {
		shared_counter++;
	}
	return NULL;
}

static void run_single_threaded(void)
{
	volatile uint64_t local = 0;
	double start = get_time_ns();
	for (uint64_t i = 0; i < ITERATIONS * 2; i++) {
		local++;
	}
	(void)local;
	double end = get_time_ns();
	printf("Single-threaded local counter:  %.3f ms (%.3f ns/iter)\n",
	       (end - start) / 1e6, (end - start) / (ITERATIONS * 2));
}

static void run_two_threads_separate(void)
{
	pthread_t t1, t2;
	counter_a = 0;
	counter_b = 0;

	double start = get_time_ns();
	pthread_create(&t1, NULL, thread_local_increment, &counter_a);
	pthread_create(&t2, NULL, thread_local_increment, &counter_b);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	double end = get_time_ns();

	printf("Two threads, separate counters: %.3f ms (%.3f ns/iter)\n",
	       (end - start) / 1e6, (end - start) / (ITERATIONS * 2));
}

static void run_two_threads_shared(void)
{
	pthread_t t1, t2;
	shared_counter = 0;

	double start = get_time_ns();
	pthread_create(&t1, NULL, thread_shared_increment, NULL);
	pthread_create(&t2, NULL, thread_shared_increment, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	double end = get_time_ns();

	printf("Two threads, shared counter:    %.3f ms (%.3f ns/iter)\n",
	       (end - start) / 1e6, (end - start) / (ITERATIONS * 2));
}

int main(void)
{
	printf("=== Cache Miss Demo ===\n");
	printf("Iterations per thread: %llu\n\n", (unsigned long long)ITERATIONS);

	run_single_threaded();
	run_two_threads_separate();
	run_two_threads_shared();

	printf("\nNote: Shared counter suffers from cache-line bouncing.\n");
	printf("Each CPU must acquire exclusive ownership of the cache line,\n");
	printf("causing expensive off-core/off-socket coherence traffic.\n");

	return 0;
}
