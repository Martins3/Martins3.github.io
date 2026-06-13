/*
 * 示例 09: 原子操作 - Compare And Swap (CAS)
 *
 * 知识点:
 * - cmpxchg 指令: 比较并交换
 * - lock 前缀: 保证原子性
 * - 内存序: acquire/release/seq_cst
 * - 实际应用: 实现自旋锁、无锁数据结构
 */

#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <stdbool.h>

/* 原子比较交换 (Compare And Swap)
 * 如果 *ptr == expected，则 *ptr = desired，返回 true
 * 否则，*expected = *ptr，返回 false
 */
static inline bool atomic_cas_int(volatile int *ptr, int *expected, int desired)
{
	int prev;
	bool success;

	__asm__ __volatile__(
		"lock; cmpxchgl %3, %1\n\t"  /* lock; cmpxchg desired, *ptr */
		"sete %0"                     /* success = (ZF == 1) */
		: "=q" (success), "=m" (*ptr), "=a" (prev)
		: "r" (desired), "m" (*ptr), "a" (*expected)
		: "memory", "cc"
		);

	if (!success)
		*expected = prev;
	return success;
}

/* 原子加载 (带 acquire 语义) */
static inline int atomic_load_acquire(volatile int *ptr)
{
	int value;

	__asm__ __volatile__(
		"movl %1, %0\n\t"
		"lock; addl $0, 0(%%rsp)"     /* 内存屏障效果 */
		: "=r" (value)
		: "m" (*ptr)
		: "memory"
		);

	return value;
}

/* 原子存储 (带 release 语义) */
static inline void atomic_store_release(volatile int *ptr, int value)
{
	__asm__ __volatile__(
		"lock; addl $0, 0(%%rsp)\n\t" /* 内存屏障效果 */
		"movl %1, %0"
		: "=m" (*ptr)
		: "r" (value)
		: "memory"
		);
}

/* 简单的自旋锁实现 */
typedef struct {
	volatile int lock;
} spinlock_t;

#define SPINLOCK_INITIALIZER {0}

static inline void spinlock_init(spinlock_t *sl)
{
	sl->lock = 0;
}

static inline void spinlock_lock(spinlock_t *sl)
{
	int expected = 0;

	while (!atomic_cas_int(&sl->lock, &expected, 1)) {
		expected = 0;
		/* 自旋时添加 pause 指令优化超线程 */
		__asm__ __volatile__("pause" ::: "memory");
	}
}

static inline void spinlock_unlock(spinlock_t *sl)
{
	atomic_store_release(&sl->lock, 0);
}

/* 测试数据 */
static volatile int counter = 0;
static spinlock_t counter_lock = SPINLOCK_INITIALIZER;

#define NUM_THREADS 4
#define INCREMENTS_PER_THREAD 100000

void *increment_worker(void *arg)
{
	(void)arg;

	for (int i = 0; i < INCREMENTS_PER_THREAD; i++) {
		spinlock_lock(&counter_lock);
		counter++;
		spinlock_unlock(&counter_lock);
	}

	return NULL;
}

int main(void)
{
	/* ===== 测试 1: 基本 CAS 操作 ===== */
	int value = 100;
	int expected = 100;
	bool success;

	success = atomic_cas_int(&value, &expected, 200);
	printf("CAS(value=100, expected=100, desired=200): %s\n",
	       success ? "success" : "failure");
	printf("value = %d (expected: 200)\n", value);
	assert(success && value == 200);

	/* 测试失败情况 */
	expected = 100;  /* 错误的期望值 */
	success = atomic_cas_int(&value, &expected, 300);
	printf("CAS(value=200, expected=100, desired=300): %s\n",
	       success ? "success" : "failure");
	printf("value = %d, expected updated to %d (expected: 200, 200)\n",
	       value, expected);
	assert(!success && value == 200 && expected == 200);

	/* ===== 测试 2: CAS 循环 (实现原子递增) ===== */
	volatile int atomic_counter = 0;

	for (int i = 0; i < 1000; i++) {
		int old_val, new_val;
		do {
			old_val = atomic_counter;
			new_val = old_val + 1;
		} while (!atomic_cas_int(&atomic_counter, &old_val, new_val));
	}

	printf("Atomic counter = %d (expected: 1000)\n", atomic_counter);
	assert(atomic_counter == 1000);

	/* ===== 测试 3: 自旋锁多线程测试 ===== */
	pthread_t threads[NUM_THREADS];

	printf("Starting %d threads, each incrementing %d times...\n",
	       NUM_THREADS, INCREMENTS_PER_THREAD);

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_create(&threads[i], NULL, increment_worker, NULL);
	}

	for (int i = 0; i < NUM_THREADS; i++) {
		pthread_join(threads[i], NULL);
	}

	printf("Final counter = %d (expected: %d)\n",
	       counter, NUM_THREADS * INCREMENTS_PER_THREAD);
	assert(counter == NUM_THREADS * INCREMENTS_PER_THREAD);

	/* ===== 测试 4: 原子交换 ===== */
	volatile int swap_val = 42;
	int old;

	__asm__ __volatile__(
		"lock; xchgl %0, %1"
		: "=r" (old), "+m" (swap_val)
		: "0" (100)              /* 新值 */
		: "memory"
		);

	printf("After xchg: old=%d, swap_val=%d (expected: 42, 100)\n",
	       old, swap_val);
	assert(old == 42 && swap_val == 100);

	/* ===== 测试 5: fetch-and-add ===== */
	volatile int fetch_val = 50;
	int prev;

	__asm__ __volatile__(
		"lock; xaddl %0, %1"
		: "=r" (prev), "+m" (fetch_val)
		: "0" (25)               /* 增加值 */
		: "memory", "cc"
		);

	printf("After xadd: prev=%d, fetch_val=%d (expected: 50, 75)\n",
	       prev, fetch_val);
	assert(prev == 50 && fetch_val == 75);

	printf("\n=== All atomic CAS tests passed! ===\n");
	return 0;
}
