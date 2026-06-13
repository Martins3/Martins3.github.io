/*
 * atomic_overhead_demo.c
 * Demonstrates the overhead of atomic operations vs normal operations.
 *
 * perfbook Table 3.1 shows that atomic CAS on a shared cache line can cost
 * hundreds of clock cycles. This program measures:
 * 1. Plain increment (no atomic, single thread)
 * 2. Atomic increment with __atomic_fetch_add (lock prefix on x86)
 * 3. CAS loop increment (compare-and-swap)
 *
 * This corresponds to the "Atomic Operations" and "Costs of Operations"
 * sections in perfbook Ch.3.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>

#define ITERATIONS 50000000ULL

static volatile uint64_t normal_counter = 0;
static volatile uint64_t atomic_counter = 0;
static volatile uint64_t cas_counter = 0;

static double get_time_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static void run_plain_increment(void)
{
	normal_counter = 0;
	double start = get_time_ns();
	for (uint64_t i = 0; i < ITERATIONS; i++) {
		normal_counter++;
	}
	(void)normal_counter;
	double end = get_time_ns();
	printf("Plain increment:                %.3f ms (%.3f ns/iter)\n",
	       (end - start) / 1e6, (end - start) / ITERATIONS);
}

static void run_atomic_increment(void)
{
	atomic_counter = 0;
	double start = get_time_ns();
	for (uint64_t i = 0; i < ITERATIONS; i++) {
		__atomic_fetch_add(&atomic_counter, 1, __ATOMIC_SEQ_CST);
	}
	(void)atomic_counter;
	double end = get_time_ns();
	printf("Atomic increment (same CPU):    %.3f ms (%.3f ns/iter)\n",
	       (end - start) / 1e6, (end - start) / ITERATIONS);
}

static void run_cas_increment(void)
{
	cas_counter = 0;
	double start = get_time_ns();
	for (uint64_t i = 0; i < ITERATIONS; i++) {
		uint64_t old, new;
		do {
			old = cas_counter;
			new = old + 1;
		} while (!__atomic_compare_exchange_n(&cas_counter, &old, new,
						      0, __ATOMIC_SEQ_CST,
						      __ATOMIC_SEQ_CST));
	}
	(void)cas_counter;
	double end = get_time_ns();
	printf("CAS loop increment (same CPU):  %.3f ms (%.3f ns/iter)\n",
	       (end - start) / 1e6, (end - start) / ITERATIONS);
}

static void *thread_atomic_worker(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < ITERATIONS; i++) {
		__atomic_fetch_add(&atomic_counter, 1, __ATOMIC_SEQ_CST);
	}
	return NULL;
}

static void run_contended_atomic(void)
{
	pthread_t t1, t2;
	atomic_counter = 0;

	double start = get_time_ns();
	pthread_create(&t1, NULL, thread_atomic_worker, NULL);
	pthread_create(&t2, NULL, thread_atomic_worker, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	double end = get_time_ns();

	printf("Atomic increment (contended):   %.3f ms (%.3f ns/iter)\n",
	       (end - start) / 1e6, (end - start) / (ITERATIONS * 2));
}

int main(void)
{
	printf("=== Atomic Overhead Demo ===\n");
	printf("Iterations: %llu\n\n", (unsigned long long)ITERATIONS);

	run_plain_increment();
	run_atomic_increment();
	run_cas_increment();
	run_contended_atomic();

	printf("\nKey observations:\n");
	printf("  - Plain increment is fastest (single instruction, no pipeline flush).\n");
	printf("  - Atomic increment uses 'lock' prefix, serializing cache access.\n");
	printf("  - CAS loop is slower due to potential retry on conflict.\n");
	printf("  - Contended atomic suffers from cache-line bouncing between CPUs.\n");

	return 0;
}
