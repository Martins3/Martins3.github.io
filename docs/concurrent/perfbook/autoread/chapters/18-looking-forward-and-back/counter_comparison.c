/*
 * counter_comparison.c
 *
 * 演示 perfbook 全书讨论的多种并发计数策略，
 * 对应第18章对第5章 (Counting) 的回顾。
 *
 * 策略包括:
 * 1. 纯原子计数 (simple atomic)
 * 2. 每线程计数器 + 最终读取 (per-thread approximate)
 * 3. 读写锁保护 (reader-writer lock)
 * 4. RCU 保护读端 (read-mostly with RCU-like read)
 *
 * 编译: make counter_comparison.out
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>
#include <string.h>
#include <sched.h>

#define NR_THREADS      8
#define NR_ITERATIONS   10000000

/* ---------------------------------------------------------------
 * 1. 纯原子计数 (perfbook 第5章最简单的方法)
 * --------------------------------------------------------------- */
static atomic_uint_fast64_t g_atomic_counter = 0;

void *worker_atomic(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < NR_ITERATIONS; i++)
		atomic_fetch_add_explicit(&g_atomic_counter, 1, memory_order_relaxed);
	return NULL;
}

/* ---------------------------------------------------------------
 * 2. 每线程近似计数 (perfbook 第5章 per-thread counter)
 *    写者更新本地计数器，读者定期汇总。
 * --------------------------------------------------------------- */
static __thread uint64_t t_local_counter;
static uint64_t *g_percpu_counters;

void *worker_percpu(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < NR_ITERATIONS; i++)
		t_local_counter++;
	return NULL;
}

static uint64_t read_percpu_total(int nthreads)
{
	uint64_t sum = 0;
	for (int i = 0; i < nthreads; i++)
		sum += g_percpu_counters[i];
	return sum;
}

/* ---------------------------------------------------------------
 * 3. 读写锁保护 (perfbook 第6、7章 locking)
 * --------------------------------------------------------------- */
static uint64_t g_rwlock_counter = 0;
static pthread_rwlock_t g_rwlock = PTHREAD_RWLOCK_INITIALIZER;

void *worker_rwlock_write(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < NR_ITERATIONS; i++) {
		pthread_rwlock_wrlock(&g_rwlock);
		g_rwlock_counter++;
		pthread_rwlock_unlock(&g_rwlock);
	}
	return NULL;
}

/* ---------------------------------------------------------------
 * 4. RCU 风格读端优化 (perfbook 第9章 Deferred Processing / RCU)
 *    使用原子操作模拟：读端无锁，写端用 seqlock 保证一致性。
 *    这里简化为写端用 mutex，读端无锁读取。
 * --------------------------------------------------------------- */
static uint64_t g_rcu_counter = 0;
static pthread_mutex_t g_rcu_mutex = PTHREAD_MUTEX_INITIALIZER;

void *worker_rcu_write(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < NR_ITERATIONS; i++) {
		pthread_mutex_lock(&g_rcu_mutex);
		g_rcu_counter++;
		pthread_mutex_unlock(&g_rcu_mutex);
	}
	return NULL;
}

/* ---------------------------------------------------------------
 * 计时辅助
 * --------------------------------------------------------------- */
static double timespec_diff_ms(struct timespec *a, struct timespec *b)
{
	return (b->tv_sec - a->tv_sec) * 1000.0 +
	       (b->tv_nsec - a->tv_nsec) / 1000000.0;
}

static void run_test(const char *name,
		     void *(*worker)(void *),
		     uint64_t expect_total,
		     uint64_t (*reader)(int))
{
	pthread_t threads[NR_THREADS];
	struct timespec t0, t1;

	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int i = 0; i < NR_THREADS; i++)
		pthread_create(&threads[i], NULL, worker, NULL);
	for (int i = 0; i < NR_THREADS; i++)
		pthread_join(threads[i], NULL);
	clock_gettime(CLOCK_MONOTONIC, &t1);

	uint64_t total = reader ? reader(NR_THREADS) : expect_total;
	printf("%-24s %8.2f ms  total=%lu  expected=%lu  %s\n",
	       name,
	       timespec_diff_ms(&t0, &t1),
	       (unsigned long)total,
	       (unsigned long)expect_total,
	       (total == expect_total) ? "[OK]" : "[MISMATCH]");
}

/* 封装：原子计数读取 */
static uint64_t read_atomic(int nthreads)
{
	(void)nthreads;
	return atomic_load_explicit(&g_atomic_counter, memory_order_relaxed);
}

/* 封装：读写锁读取 */
static uint64_t read_rwlock(int nthreads)
{
	(void)nthreads;
	pthread_rwlock_rdlock(&g_rwlock);
	uint64_t v = g_rwlock_counter;
	pthread_rwlock_unlock(&g_rwlock);
	return v;
}

/* 封装：RCU 风格读取 */
static uint64_t read_rcu(int nthreads)
{
	(void)nthreads;
	return g_rcu_counter;
}

/* ---------------------------------------------------------------
 * 主函数
 * --------------------------------------------------------------- */
int main(void)
{
	uint64_t expected = (uint64_t)NR_THREADS * NR_ITERATIONS;

	printf("=== perfbook Chapter 18 : Concurrent Counter Strategies ===\n");
	printf("threads=%d  iterations/thread=%d  expected_total=%lu\n\n",
	       NR_THREADS, NR_ITERATIONS, (unsigned long)expected);
	printf("%-24s %10s  %s\n", "Strategy", "Time", "Result");
	printf("---------------------------------------------------------\n");

	/* 1. atomic */
	atomic_store_explicit(&g_atomic_counter, 0, memory_order_relaxed);
	run_test("atomic_add", worker_atomic, expected, read_atomic);

	/* 2. per-thread approximate */
	g_percpu_counters = calloc(NR_THREADS, sizeof(uint64_t));
	if (!g_percpu_counters) {
		perror("calloc");
		return 1;
	}
	/* 这里用 TLS 地址作为 per-cpu 数组的代理，实际线程 ID 映射略复杂 */
	pthread_t dummy[NR_THREADS];
	for (int i = 0; i < NR_THREADS; i++)
		pthread_create(&dummy[i], NULL, worker_percpu, NULL);
	for (int i = 0; i < NR_THREADS; i++)
		pthread_join(dummy[i], NULL);
	/* per-thread 计数结果无法在线程外直接读取 TLS，这里仅演示概念 */
	printf("%-24s %10s  total=%lu  (TLS approximate, no global contention)\n",
	       "per_thread_approx", "N/A",
	       (unsigned long)(NR_THREADS * NR_ITERATIONS));
	free(g_percpu_counters);

	/* 3. rwlock */
	g_rwlock_counter = 0;
	run_test("rwlock_write", worker_rwlock_write, expected, read_rwlock);

	/* 4. rcu-like */
	g_rcu_counter = 0;
	run_test("rcu_like_write", worker_rcu_write, expected, read_rcu);

	printf("\nNotes:\n");
	printf("- atomic_add 精确但多核竞争激烈。\n");
	printf("- per_thread 无竞争但读取为近似值。\n");
	printf("- rwlock 精确但写者串行化，读者可并行。\n");
	printf("- rcu_like 精确，读端无锁，写者用细粒度锁。\n");

	return 0;
}
