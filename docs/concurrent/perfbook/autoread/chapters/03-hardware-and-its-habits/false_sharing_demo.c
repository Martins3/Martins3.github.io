/*
 * # false share 的原理和避免方法
 * <!-- 9b08fad3-d98d-4805-b14b-d64bbc37101b -->*
 *
 * https://cloud.tencent.com/developer/article/1844992
 * 假共享 : https://www.kernel.org/doc/html/latest/kernel-hacking/false-sharing.html
 *
 * 这个和 numa socket 之类的没有关系发
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>

#define ITERATIONS 100000000ULL

/*
 * Structure with tight packing - all fields on same cache line.
 * On x86_64, cache line is typically 64 bytes.
 */
struct packed_counters {
	uint64_t a;
	uint64_t b;
};

/*
 * Structure with padding - each field on its own cache line.
 */
struct padded_counters {
	uint64_t a;
	uint64_t pad[7];  /* 64 bytes total for a */
	uint64_t b;
	uint64_t pad2[7]; /* 64 bytes total for b */
};

static struct packed_counters packed __attribute__((aligned(64)));
static struct padded_counters padded __attribute__((aligned(64)));
static volatile uint64_t dummy_sink;

static double get_time_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static void *thread_inc_a_packed(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < ITERATIONS; i++) {
		packed.a++;
	}
	dummy_sink = packed.a;
	return NULL;
}

static void *thread_inc_b_packed(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < ITERATIONS; i++) {
		packed.b++;
	}
	dummy_sink = packed.b;
	return NULL;
}

static void *thread_inc_a_padded(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < ITERATIONS; i++) {
		padded.a++;
	}
	dummy_sink = padded.a;
	return NULL;
}

static void *thread_inc_b_padded(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < ITERATIONS; i++) {
		padded.b++;
	}
	dummy_sink = padded.b;
	return NULL;
}

static void run_false_sharing(void)
{
	pthread_t t1, t2;
	packed.a = 0;
	packed.b = 0;

	double start = get_time_ns();
	pthread_create(&t1, NULL, thread_inc_a_packed, NULL);
	pthread_create(&t2, NULL, thread_inc_b_packed, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	double end = get_time_ns();

	printf("False sharing (packed):         %.3f ms (%.3f ns/iter)\n",
	       (end - start) / 1e6, (end - start) / (ITERATIONS * 2));
}

static void run_no_false_sharing(void)
{
	pthread_t t1, t2;
	padded.a = 0;
	padded.b = 0;

	double start = get_time_ns();
	pthread_create(&t1, NULL, thread_inc_a_padded, NULL);
	pthread_create(&t2, NULL, thread_inc_b_padded, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	double end = get_time_ns();

	printf("No false sharing (padded):      %.3f ms (%.3f ns/iter)\n",
	       (end - start) / 1e6, (end - start) / (ITERATIONS * 2));
}

int main(void)
{
	printf("=== False Sharing Demo ===\n");
	printf("Iterations per thread: %llu\n\n", (unsigned long long)ITERATIONS);

	run_false_sharing();
	run_no_false_sharing();

	printf("\nExplanation:\n");
	printf("  packed_counters.a and .b sit on the same cache line.\n");
	printf("  Each increment by one CPU invalidates the other CPU's cache,\n");
	printf("  causing the cache line to bounce back and forth.\n");
	printf("  padded_counters places a and b on separate cache lines,\n");
	printf("  eliminating this overhead.\n");

	return 0;
}
