/*
 * branch_prediction_demo.c
 * Demonstrates the impact of branch prediction on CPU pipeline performance.
 *
 * perfbook Ch.3 "Pipelined CPUs" explains that unpredictable control flow
 * causes pipeline flushes. This program compares:
 * 1. Predictable branches (sorted data, monotonic pattern)
 * 2. Unpredictable branches (random data)
 *
 * The performance difference shows the cost of branch misprediction and
 * pipeline flushes.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#define ARRAY_SIZE (64 * 1024 * 1024)
#define ITERATIONS 10

static int *data;

static double get_time_ns(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

static int cmp_int(const void *a, const void *b)
{
	return (*(const int *)a - *(const int *)b);
}

static uint64_t count_positive_sorted(void)
{
	uint64_t count = 0;
	for (size_t i = 0; i < ARRAY_SIZE; i++) {
		if (data[i] > 0) {
			count++;
		}
	}
	return count;
}

static uint64_t count_positive_random(void)
{
	uint64_t count = 0;
	for (size_t i = 0; i < ARRAY_SIZE; i++) {
		if (data[i] > 0) {
			count++;
		}
	}
	return count;
}

int main(void)
{
	printf("=== Branch Prediction Demo ===\n");
	printf("Array size: %d elements\n\n", ARRAY_SIZE);

	data = malloc(ARRAY_SIZE * sizeof(int));
	if (!data) {
		perror("malloc");
		return 1;
	}

	/* Initialize with random values (50/50 positive/negative) */
	srand(42);
	for (size_t i = 0; i < ARRAY_SIZE; i++) {
		data[i] = (rand() % 2) ? 1 : -1;
	}

	/* Warmup */
	count_positive_random();

	/* Test with unpredictable (random) data */
	double total_random = 0;
	for (int i = 0; i < ITERATIONS; i++) {
		double start = get_time_ns();
		volatile uint64_t count = count_positive_random();
		double end = get_time_ns();
		total_random += (end - start);
		(void)count;
	}

	/* Sort to make branches perfectly predictable */
	qsort(data, ARRAY_SIZE, sizeof(int), cmp_int);

	/* Test with predictable (sorted) data */
	double total_sorted = 0;
	for (int i = 0; i < ITERATIONS; i++) {
		double start = get_time_ns();
		volatile uint64_t count = count_positive_sorted();
		double end = get_time_ns();
		total_sorted += (end - start);
		(void)count;
	}

	printf("Random data (unpredictable):    %.3f ms avg\n",
	       total_random / ITERATIONS / 1e6);
	printf("Sorted data (predictable):      %.3f ms avg\n",
	       total_sorted / ITERATIONS / 1e6);
	printf("Speedup from prediction:        %.2fx\n",
	       total_random / total_sorted);

	printf("\nExplanation:\n");
	printf("  With sorted data, CPU branch predictor learns the pattern:\n");
	printf("  first half positive (take branch), second half negative (skip).\n");
	printf("  With random data, predictor is wrong ~50%% of time, causing\n");
	printf("  pipeline flushes and drastically reducing throughput.\n");

	free(data);
	return 0;
}
