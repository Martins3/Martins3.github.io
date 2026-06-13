/*
 * count_nonatomic.c: 演示非原子计数器在多线程下的数据丢失
 *
 * 对应 perfbook 第 5 章 "Why Isn't Concurrent Counting Trivial?"
 * 展示简单的 READ_ONCE/WRITE_ONCE 计数在并发下会丢失更新。
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
	/* 模拟 perfbook 中的 count_nonatomic.c */
	__atomic_store_n(&counter,
		__atomic_load_n(&counter, __ATOMIC_RELAXED) + 1,
		__ATOMIC_RELAXED);
}

static unsigned long read_count(void)
{
	return __atomic_load_n(&counter, __ATOMIC_RELAXED);
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

	printf("Test: non-atomic counter with %d threads, %d increments each\n",
		NR_THREADS, NR_INCREMENTS);

	for (i = 0; i < NR_THREADS; i++) {
		if (pthread_create(&threads[i], NULL, thread_func, NULL) != 0) {
			perror("pthread_create");
			exit(EXIT_FAILURE);
		}
	}

	for (i = 0; i < NR_THREADS; i++)
		pthread_join(threads[i], NULL);

	unsigned long expected = (unsigned long)NR_THREADS * NR_INCREMENTS;
	unsigned long actual = read_count();
	printf("Expected: %lu, Actual: %lu, Lost: %lu (%.2f%%)\n",
		expected, actual,
		expected - actual,
		100.0 * (expected - actual) / expected);

	return 0;
}
