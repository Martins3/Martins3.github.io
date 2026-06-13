/*
 * count_atomic.c: 演示原子计数器正确但扩展性差
 *
 * 对应 perfbook 第 5 章 "Just Count Atomically!"
 * 使用 __atomic_fetch_add 保证正确性，但多核竞争激烈。
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/time.h>

#define NR_THREADS 4
#define NR_INCREMENTS 10000000

static unsigned long counter = 0;

static inline void inc_count(void)
{
	__atomic_fetch_add(&counter, 1, __ATOMIC_SEQ_CST);
}

static unsigned long read_count(void)
{
	return __atomic_load_n(&counter, __ATOMIC_SEQ_CST);
}

static double gettime(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return tv.tv_sec + tv.tv_usec / 1e6;
}

static void *thread_func(void *arg)
{
	long i;
	(void)arg;
	for (i = 0; i < NR_INCREMENTS; i++)
		inc_count();
	return NULL;
}

int main(void)
{
	pthread_t threads[NR_THREADS];
	int i;
	double t0, t1;

	printf("Test: atomic counter with %d threads, %d increments each\n",
		NR_THREADS, NR_INCREMENTS);

	t0 = gettime();
	for (i = 0; i < NR_THREADS; i++) {
		if (pthread_create(&threads[i], NULL, thread_func, NULL) != 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}

	for (i = 0; i < NR_THREADS; i++)
		pthread_join(threads[i], NULL);
	t1 = gettime();

	unsigned long expected = (unsigned long)NR_THREADS * NR_INCREMENTS;
	unsigned long actual = read_count();
	printf("Expected: %lu, Actual: %lu, Time: %.3fs\n",
		expected, actual, t1 - t0);

	return 0;
}
