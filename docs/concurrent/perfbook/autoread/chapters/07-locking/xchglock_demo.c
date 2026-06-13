/*
 * xchglock_demo.c: 基于原子交换的独占锁实现
 *
 * 对应 perfbook 第7章:
 *  - 7.3.1 Sample Exclusive-Locking Implementation Based on Atomic Exchange
 *
 * 这是一个教学用的自旋锁实现，展示了锁的核心原理。
 * Linux 内核实际使用的是更复杂的 queued spinlock (qspinlock)。
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdatomic.h>
#include <time.h>

/* ========== 基于原子交换的锁 ========== */

typedef atomic_int xchglock_t;

#define XCHG_LOCK_INIT 0

static inline void xchg_lock(xchglock_t *xp)
{
	/*
	 * 外循环: 尝试原子交换锁值为 1 (locked)
	 * 如果返回旧值 1，说明锁已被持有。
	 */
	while (atomic_exchange_explicit(xp, 1, memory_order_acquire) == 1) {
		/*
		 * 内循环: 自旋等待锁变为可用。
		 * 这里使用 load 而非 exchange 以减少缓存一致性流量
		 * (ping-pong cache line)。
		 */
		while (atomic_load_explicit(xp, memory_order_relaxed) == 1)
			__asm__ volatile ("pause" ::: "memory");
	}
}

static inline void xchg_unlock(xchglock_t *xp)
{
	/*
	 * 原子交换 0 (unlocked) 到锁中。
	 * xchg 隐含完整的内存屏障，因此不需要额外 barrier。
	 * 在 C11 atomic 中，我们使用 release 语义。
	 */
	atomic_exchange_explicit(xp, 0, memory_order_release);
}

/* ========== 测试 ========== */

#define NTHREADS 4
#define NINCREMENTS 1000000

xchglock_t counter_lock = XCHG_LOCK_INIT;
unsigned long shared_counter = 0;

void *worker(void *arg)
{
	int id = *(int *)arg;
	(void)id;

	for (int i = 0; i < NINCREMENTS; i++) {
		xchg_lock(&counter_lock);
		shared_counter++;
		xchg_unlock(&counter_lock);
	}
	return NULL;
}

/* 对比: 无锁 (错误) */
unsigned long unsafe_counter = 0;

void *unsafe_worker(void *arg)
{
	(void)arg;
	for (int i = 0; i < NINCREMENTS; i++) {
		unsafe_counter++;
	}
	return NULL;
}

/* 对比: C11 mutex */
pthread_mutex_t pthread_lock = PTHREAD_MUTEX_INITIALIZER;
unsigned long pthread_counter = 0;

void *pthread_worker(void *arg)
{
	(void)arg;
	for (int i = 0; i < NINCREMENTS; i++) {
		pthread_mutex_lock(&pthread_lock);
		pthread_counter++;
		pthread_mutex_unlock(&pthread_lock);
	}
	return NULL;
}

static double get_time_ms(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main(void)
{
	pthread_t threads[NTHREADS];
	int ids[NTHREADS];
	double start, end;

	printf("=== Atomic-Exchange Lock Demo ===\n\n");

	/* 1. 测试正确性 */
	printf("--- 1. Correctness Test ---\n");
	shared_counter = 0;
	for (int i = 0; i < NTHREADS; i++) {
		ids[i] = i;
		pthread_create(&threads[i], NULL, worker, &ids[i]);
	}
	for (int i = 0; i < NTHREADS; i++)
		pthread_join(threads[i], NULL);
	printf("[xchglock] Expected: %lu, Actual: %lu, %s\n",
	       (unsigned long)NTHREADS * NINCREMENTS,
	       shared_counter,
	       shared_counter == (unsigned long)NTHREADS * NINCREMENTS ? "PASS" : "FAIL");

	/* 无锁错误版本 */
	unsafe_counter = 0;
	for (int i = 0; i < NTHREADS; i++)
		pthread_create(&threads[i], NULL, unsafe_worker, NULL);
	for (int i = 0; i < NTHREADS; i++)
		pthread_join(threads[i], NULL);
	printf("[no-lock] Expected: %lu, Actual: %lu, %s (racy!)\n",
	       (unsigned long)NTHREADS * NINCREMENTS,
	       unsafe_counter,
	       unsafe_counter == (unsigned long)NTHREADS * NINCREMENTS ? "PASS" : "FAIL");

	/* pthread mutex */
	pthread_counter = 0;
	for (int i = 0; i < NTHREADS; i++)
		pthread_create(&threads[i], NULL, pthread_worker, NULL);
	for (int i = 0; i < NTHREADS; i++)
		pthread_join(threads[i], NULL);
	printf("[pthread]  Expected: %lu, Actual: %lu, %s\n\n",
	       (unsigned long)NTHREADS * NINCREMENTS,
	       pthread_counter,
	       pthread_counter == (unsigned long)NTHREADS * NINCREMENTS ? "PASS" : "FAIL");

	/* 2. 性能对比 */
	printf("--- 2. Performance Comparison ---\n");

	/* xchglock */
	shared_counter = 0;
	start = get_time_ms();
	for (int i = 0; i < NTHREADS; i++)
		pthread_create(&threads[i], NULL, worker, &ids[i]);
	for (int i = 0; i < NTHREADS; i++)
		pthread_join(threads[i], NULL);
	end = get_time_ms();
	printf("[xchglock] Time: %.3f ms\n", end - start);

	/* pthread mutex */
	pthread_counter = 0;
	start = get_time_ms();
	for (int i = 0; i < NTHREADS; i++)
		pthread_create(&threads[i], NULL, pthread_worker, NULL);
	for (int i = 0; i < NTHREADS; i++)
		pthread_join(threads[i], NULL);
	end = get_time_ms();
	printf("[pthread]  Time: %.3f ms\n", end - start);

	printf("\nNote: xchglock is a simple test-and-set lock.\n");
	printf("      Linux kernel uses qspinlock (MCS-based queued lock)\n");
	printf("      which has better fairness and scalability under contention.\n");

	return 0;
}
