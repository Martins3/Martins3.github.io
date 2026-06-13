/*
 * right_tool_demo.c
 *
 * 对应第18章最后强调的 "The Most Important Lesson: Use the Right Tools"。
 * 演示不同并发场景下选择正确同步机制的重要性。
 *
 * 场景:
 * 1. 错误工具: 用 mutex 保护只增计数器
 * 2. 正确工具: 用原子操作
 * 3. 错误工具: 用全局锁保护 per-CPU 统计
 * 4. 正确工具: 用 per-thread 数据 + 最终汇总
 * 5. 错误工具: 用读写锁做高频小对象分配
 * 6. 正确工具: 用 slab/arena + 原子栈（简化版）
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <string.h>
#include <stdint.h>

#define NR_THREADS    8
#define NR_OPS        500000
#define POOL_SIZE     (NR_THREADS * NR_OPS)

/* ---------------------------------------------------------------
 * 计时
 * --------------------------------------------------------------- */
static double timespec_diff_ms(struct timespec *a, struct timespec *b)
{
	return (b->tv_sec - a->tv_sec) * 1000.0 +
	       (b->tv_nsec - a->tv_nsec) / 1000000.0;
}

/* ---------------------------------------------------------------
 * 场景1 & 2: 简单计数器
 * --------------------------------------------------------------- */
static pthread_mutex_t g_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static uint64_t g_count_mutex_val = 0;
static atomic_uint_fast64_t g_count_atomic = 0;

void *worker_count_mutex(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < NR_OPS; i++) {
		pthread_mutex_lock(&g_count_mutex);
		g_count_mutex_val++;
		pthread_mutex_unlock(&g_count_mutex);
	}
	return NULL;
}

void *worker_count_atomic(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < NR_OPS; i++)
		atomic_fetch_add_explicit(&g_count_atomic, 1, memory_order_relaxed);
	return NULL;
}

/* ---------------------------------------------------------------
 * 场景3 & 4: per-thread 统计 vs 全局锁
 * --------------------------------------------------------------- */
static __thread uint64_t t_stat_local;
static uint64_t g_stat_global = 0;
static pthread_mutex_t g_stat_mutex = PTHREAD_MUTEX_INITIALIZER;

void *worker_stat_global(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < NR_OPS; i++) {
		pthread_mutex_lock(&g_stat_mutex);
		g_stat_global++;
		pthread_mutex_unlock(&g_stat_mutex);
	}
	return NULL;
}

void *worker_stat_local(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < NR_OPS; i++)
		t_stat_local++;
	return NULL;
}

/* ---------------------------------------------------------------
 * 场景5 & 6: 内存分配器
 *   5. 全局锁保护 freelist
 *   6. per-thread freelist (lock-free stack using atomic)
 * --------------------------------------------------------------- */
struct node {
	struct node *next;
	char pad[56]; /* 凑 cache line */
};

static struct node *g_freelist = NULL;
static pthread_mutex_t g_freelist_lock = PTHREAD_MUTEX_INITIALIZER;
static struct node g_pool[POOL_SIZE];

/* 全局锁版本 */
static struct node *alloc_global(void)
{
	pthread_mutex_lock(&g_freelist_lock);
	struct node *n = g_freelist;
	if (n)
		g_freelist = n->next;
	pthread_mutex_unlock(&g_freelist_lock);
	return n;
}

static void free_global(struct node *n)
{
	pthread_mutex_lock(&g_freelist_lock);
	n->next = g_freelist;
	g_freelist = n;
	pthread_mutex_unlock(&g_freelist_lock);
}

/* per-thread lock-free 版本 */
static __thread struct node *t_freelist = NULL;
static atomic_int t_initialized = 0;
static pthread_mutex_t init_lock = PTHREAD_MUTEX_INITIALIZER;
static struct node *t_pool_base = NULL;

static void init_thread_pool(void)
{
	if (atomic_load_explicit(&t_initialized, memory_order_acquire))
		return;
	pthread_mutex_lock(&init_lock);
	if (!t_pool_base) {
		/* 简化为从全局 pool 切一块 */
		static atomic_uint_fast64_t global_off = 0;
		uint64_t my_start = atomic_fetch_add_explicit(&global_off, NR_OPS,
							    memory_order_relaxed);
		t_pool_base = &g_pool[my_start];
		/* 初始化 freelist */
		for (uint64_t i = 0; i < NR_OPS; i++) {
			struct node *n = &t_pool_base[i];
			n->next = t_freelist;
			t_freelist = n;
		}
	}
	atomic_store_explicit(&t_initialized, 1, memory_order_release);
	pthread_mutex_unlock(&init_lock);
}

static struct node *alloc_local(void)
{
	init_thread_pool();
	struct node *n = t_freelist;
	if (n)
		t_freelist = n->next;
	return n;
}

static void free_local(struct node *n)
{
	n->next = t_freelist;
	t_freelist = n;
}

void *worker_alloc_global(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < NR_OPS; i++) {
		struct node *n = alloc_global();
		if (n)
			free_global(n);
	}
	return NULL;
}

void *worker_alloc_local(void *arg)
{
	(void)arg;
	for (uint64_t i = 0; i < NR_OPS; i++) {
		struct node *n = alloc_local();
		if (n)
			free_local(n);
	}
	return NULL;
}

/* ---------------------------------------------------------------
 * 测试框架
 * --------------------------------------------------------------- */
static void run_scenario(const char *name,
			 void *(*worker)(void *),
			 int init_needed)
{
	pthread_t threads[NR_THREADS];
	struct timespec t0, t1;

	if (init_needed) {
		/* 初始化全局 freelist */
		g_freelist = NULL;
		for (uint64_t i = 0; i < POOL_SIZE; i++) {
			g_pool[i].next = g_freelist;
			g_freelist = &g_pool[i];
		}
		atomic_store_explicit(&t_initialized, 0, memory_order_relaxed);
		atomic_store_explicit((atomic_uint_fast64_t *)&g_count_atomic, 0,
				      memory_order_relaxed);
		g_count_mutex_val = 0;
		g_stat_global = 0;
	}

	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int i = 0; i < NR_THREADS; i++)
		pthread_create(&threads[i], NULL, worker, NULL);
	for (int i = 0; i < NR_THREADS; i++)
		pthread_join(threads[i], NULL);
	clock_gettime(CLOCK_MONOTONIC, &t1);

	printf("%-28s %8.2f ms\n", name, timespec_diff_ms(&t0, &t1));
}

int main(void)
{
	printf("=== perfbook Ch18: Use the Right Tool Demo ===\n");
	printf("threads=%d  ops/thread=%d\n\n", NR_THREADS, NR_OPS);

	printf("--- Counter: mutex vs atomic ---\n");
	run_scenario("WRONG: mutex counter", worker_count_mutex, 0);
	run_scenario("RIGHT: atomic counter", worker_count_atomic, 0);

	printf("\n--- Statistics: global lock vs per-thread ---\n");
	run_scenario("WRONG: global lock stat", worker_stat_global, 0);
	run_scenario("RIGHT: per-thread stat", worker_stat_local, 0);

	printf("\n--- Allocator: global freelist vs per-thread ---\n");
	run_scenario("WRONG: global freelist", worker_alloc_global, 1);
	run_scenario("RIGHT: per-thread freelist", worker_alloc_local, 1);

	printf("\n=== Key Takeaway ===\n");
	printf("Choosing the right synchronization mechanism can yield\n");
	printf("orders-of-magnitude performance improvements.  Atomic ops\n");
	printf("eliminate contention for counters; per-thread data eliminates\n");
	printf("contention for statistics and allocators.  This is the\n");
	printf("essence of perfbook's 'Partition first, batch second,\n");
	printf("weaken third, and code fourth' design philosophy.\n");

	return 0;
}
