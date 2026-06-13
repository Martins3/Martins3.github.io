/*
 * heisenbug_demo.c: Demonstrate anti-heisenbug techniques.
 *
 * Based on Section 11.5.2.1 "Add Delay" from perfbook Chapter 11.
 *
 * Race conditions are often low-probability events. Adding delay in
 * critical sections can increase the failure rate, making the bug
 * easier to observe and fix.
 *
 * This demo shows a simple load-add-store race. Without delay, the
 * failure rate may be very low on some systems. With delay inserted
 * between load and store, the failure rate increases dramatically.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

static volatile unsigned long counter = 0;
static unsigned long delay_ns = 0;
static int nr_threads = 4;
static unsigned long nr_inc_per_thread = 100000;

static uint64_t getns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static void *inc_count(void *arg)
{
	unsigned long i;
	(void)arg;

	for (i = 0; i < nr_inc_per_thread; i++) {
		unsigned long tmp = counter;
		if (delay_ns) {
			struct timespec ts = { 0, (long)delay_ns };
			nanosleep(&ts, NULL);
		}
		counter = tmp + 1;
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	pthread_t *threads;
	unsigned long expected;
	unsigned long actual;
	unsigned long lost;
	uint64_t start, end;
	int i;

	if (argc > 1)
		nr_threads = atoi(argv[1]);
	if (argc > 2)
		nr_inc_per_thread = strtoul(argv[2], NULL, 0);
	if (argc > 3)
		delay_ns = strtoul(argv[3], NULL, 0);

	printf("threads=%d increments=%lu delay_ns=%lu\n",
	       nr_threads, nr_inc_per_thread, delay_ns);

	threads = malloc(sizeof(*threads) * nr_threads);
	if (!threads) {
		perror("malloc");
		return 1;
	}

	counter = 0;
	start = getns();

	for (i = 0; i < nr_threads; i++) {
		if (pthread_create(&threads[i], NULL, inc_count, NULL) != 0) {
			perror("pthread_create");
			return 1;
		}
	}

	for (i = 0; i < nr_threads; i++)
		pthread_join(threads[i], NULL);

	end = getns();
	expected = nr_threads * nr_inc_per_thread;
	actual = counter;
	lost = expected - actual;

	printf("expected=%lu actual=%lu lost=%lu (%.2f%%)\n",
	       expected, actual, lost,
	       expected ? (100.0 * lost / expected) : 0.0);
	printf("elapsed_ms=%llu\n", (unsigned long long)(end - start) / 1000000);

	free(threads);
	return 0;
}
