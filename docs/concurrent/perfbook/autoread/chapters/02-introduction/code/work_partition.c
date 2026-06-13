/*
 * work_partition.c
 * 演示工作划分(Work Partitioning)对性能的影响
 *
 * 对应 perfbook 第 2 章 sec:intro:Work Partitioning
 * 核心观点：不均匀划分会导致负载失衡，影响可扩展性
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#define N_THREADS 4
#define TOTAL_WORK 400000000UL

static uint64_t timespec_diff_ns(struct timespec *a, struct timespec *b)
{
	return (uint64_t)(b->tv_sec - a->tv_sec) * 1000000000ULL +
	       (uint64_t)(b->tv_nsec - a->tv_nsec);
}

/* 模拟 CPU 密集型工作 */
static void do_work(uint64_t iterations)
{
	volatile uint64_t sum = 0;
	for (uint64_t i = 0; i < iterations; i++)
		sum += i;
	(void)sum;
}

/* 均匀划分：每个线程等量工作 */
struct partition_args {
	uint64_t start;
	uint64_t end;
};

static void *thread_uniform(void *arg)
{
	struct partition_args *p = arg;
	do_work(p->end - p->start);
	return NULL;
}

/* 不均匀划分：一个线程做大部分工作 */
static void *thread_uneven(void *arg)
{
	struct partition_args *p = arg;
	do_work(p->end - p->start);
	return NULL;
}

static double run_test(void *(*thread_fn)(void *),
		       uint64_t *work_per_thread)
{
	pthread_t threads[N_THREADS];
	struct partition_args args[N_THREADS];
	struct timespec t0, t1;

	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int i = 0; i < N_THREADS; i++) {
		args[i].start = 0;
		args[i].end = work_per_thread[i];
		pthread_create(&threads[i], NULL, thread_fn, &args[i]);
	}
	for (int i = 0; i < N_THREADS; i++)
		pthread_join(threads[i], NULL);
	clock_gettime(CLOCK_MONOTONIC, &t1);

	return (double)timespec_diff_ns(&t0, &t1) / 1000000.0; /* ms */
}

int main(void)
{
	uint64_t work_per_thread[N_THREADS];
	double t_uniform, t_uneven;

	printf("Work Partitioning Demo (threads=%d, total work=%luM iters)\n",
	       N_THREADS, TOTAL_WORK / 1000000);
	printf("---------------------------------------------------------\n");

	/* 均匀划分 */
	uint64_t per_thread = TOTAL_WORK / N_THREADS;
	for (int i = 0; i < N_THREADS; i++)
		work_per_thread[i] = per_thread;
	t_uniform = run_test(thread_uniform, work_per_thread);
	printf("Uniform partition:  %.3f ms (each thread %luM)\n",
	       t_uniform, per_thread / 1000000);

	/* 不均匀划分：最后一个线程做一半工作 */
	for (int i = 0; i < N_THREADS - 1; i++)
		work_per_thread[i] = TOTAL_WORK / (2 * (N_THREADS - 1));
	work_per_thread[N_THREADS - 1] = TOTAL_WORK / 2;
	t_uneven = run_test(thread_uneven, work_per_thread);
	printf("Uneven partition:   %.3f ms (last thread %luM, others %luM)\n",
	       t_uneven, work_per_thread[N_THREADS - 1] / 1000000,
	       work_per_thread[0] / 1000000);

	printf("Speedup loss due to uneven partition: %.1f%%\n",
	       (t_uneven - t_uniform) / t_uniform * 100.0);

	return 0;
}
