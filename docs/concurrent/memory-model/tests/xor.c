/*
 * XOR (read-modify-write 不是原子的)
 *
 * 两个 thread 分别对同一个 unsigned long 的不同 bit 做 v ^= BIT。
 * 直觉: "对一个 long 的读写总是原子的, 两个线程动的是不同的 bit,
 *       互不影响"。这个直觉是错的:
 *   - 单次对齐的 load 或 store 确实是原子的 (不会撕裂,
 *     不会看到半新半旧的值), x86 和 ARM 都保证;
 *   - 但 v ^= BIT 是 load -> modify -> store 三步, 三步之间可以插入
 *     另一个线程的完整 RMW, 后写的一方把先写的一方的结果覆盖掉,
 *     即 lost update。
 *
 * 每个 thread 把自己的 bit 翻转偶数次, 正确结果必须是 0;
 * 任何 lost update 都会改变某个 bit 的翻转次数, 最终值非 0。
 *
 * plain 版本: v ^= bit, 普通 load/store;
 * atomic 版本 (编译时 -DUSE_FENCE, 复用现有的双版本机制):
 *   __atomic_fetch_xor, x86 上是 lock xor, ARM 上是 ldaxr/stlxr 循环,
 *   整个 RMW 原子完成, 结果恒为 0。
 */
#include "common.h"

#define FLIPS 500000UL /* 每个线程每轮翻转次数, 必须是偶数 */

#ifdef USE_FENCE
#define MODE "atomic"
#else
#define MODE "plain"
#endif

static volatile unsigned long v PAD_LINE;

static void *worker(void *arg)
{
	unsigned long bit = 1UL << (int)(long)arg;

	for (unsigned long i = 0; i < FLIPS; i++) {
#ifdef USE_FENCE
		__atomic_fetch_xor(&v, bit, __ATOMIC_SEQ_CST);
#else
		v ^= bit;
#endif
	}
	return NULL;
}

int main(int argc, char **argv)
{
	int trials = argc > 1 ? atoi(argv[1]) : 50;
	int bad = 0;

	for (int t = 0; t < trials; t++) {
		pthread_t t0, t1;

		v = 0;
		compiler_barrier();

		pthread_create(&t0, NULL, worker, (void *)0);
		pthread_create(&t1, NULL, worker, (void *)1);
		pthread_join(t0, NULL);
		pthread_join(t1, NULL);

		if (v != 0) {
			bad++;
			printf("  trial %d: final value %#lx (expect 0)\n", t, v);
		}
	}

	printf("[xor-%s] arch=%s: lost-update in %d / %d trials (each thread %lu flips)\n",
	       MODE, ARCH_NAME, bad, trials, FLIPS);
	return 0;
}
