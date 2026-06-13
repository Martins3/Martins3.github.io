/*
 * microopt_demo.c - Demonstrate cache-line alignment effects.
 * Corresponds to perfbook Section 13.6 "Micro-Optimization".
 *
 * When a frequently updated counter shares a cache line with read-mostly
 * list elements, readers suffer cache misses.  Aligning the counter to
 * a separate cache line fixes this.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <pthread.h>
#include <time.h>
#include <stdatomic.h>

#ifndef CACHE_LINE_SIZE
#define CACHE_LINE_SIZE 64
#endif

struct bad_elem {
	struct bad_elem *next;
	long data;
	atomic_long counter;  /* shares cache line with next/data */
};

struct good_elem {
	struct good_elem *next;
	long data;
	char pad[CACHE_LINE_SIZE - sizeof(void *) - sizeof(long)];
	atomic_long counter;  /* on its own cache line */
};

static struct bad_elem *bad_list = NULL;
static struct good_elem *good_list = NULL;
static const int N_ELEMS = 1000;
static const int N_ITERS = 100000;

static double now_sec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static void *bad_updater(void *arg)
{
	(void)arg;
	for (int i = 0; i < N_ITERS; i++) {
		for (struct bad_elem *e = bad_list; e; e = e->next)
			atomic_fetch_add_explicit(&e->counter, 1, memory_order_relaxed);
	}
	return NULL;
}

static void *bad_reader(void *arg)
{
	(void)arg;
	long sum = 0;
	for (int i = 0; i < N_ITERS; i++) {
		for (struct bad_elem *e = bad_list; e; e = e->next)
			sum += e->data;
	}
	return (void *)(uintptr_t)sum;
}

static void *good_updater(void *arg)
{
	(void)arg;
	for (int i = 0; i < N_ITERS; i++) {
		for (struct good_elem *e = good_list; e; e = e->next)
			atomic_fetch_add_explicit(&e->counter, 1, memory_order_relaxed);
	}
	return NULL;
}

static void *good_reader(void *arg)
{
	(void)arg;
	long sum = 0;
	for (int i = 0; i < N_ITERS; i++) {
		for (struct good_elem *e = good_list; e; e = e->next)
			sum += e->data;
	}
	return (void *)(uintptr_t)sum;
}

int main(void)
{
	printf("[microopt_demo] Cache-line alignment effect\n");
	printf("  CACHE_LINE_SIZE = %d\n", CACHE_LINE_SIZE);
	printf("  sizeof(bad_elem)  = %zu (counter shares line)\n",
	       sizeof(struct bad_elem));
	printf("  sizeof(good_elem) = %zu (counter on separate line)\n",
	       sizeof(struct good_elem));

	/* Build lists */
	for (int i = 0; i < N_ELEMS; i++) {
		struct bad_elem *be = calloc(1, sizeof(*be));
		be->data = i;
		be->next = bad_list;
		bad_list = be;

		struct good_elem *ge = calloc(1, sizeof(*ge));
		ge->data = i;
		ge->next = good_list;
		good_list = ge;
	}

	pthread_t t1, t2;

	double t0 = now_sec();
	pthread_create(&t1, NULL, bad_reader, NULL);
	pthread_create(&t2, NULL, bad_updater, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	double bad_time = now_sec() - t0;
	printf("  bad layout  time = %.3f sec\n", bad_time);

	t0 = now_sec();
	pthread_create(&t1, NULL, good_reader, NULL);
	pthread_create(&t2, NULL, good_updater, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	double good_time = now_sec() - t0;
	printf("  good layout time = %.3f sec\n", good_time);

	if (good_time < bad_time)
		printf("[OK] Aligned layout is %.2fx faster\n", bad_time / good_time);
	else
		printf("[INFO] Alignment had little effect on this machine\n");

	return 0;
}
