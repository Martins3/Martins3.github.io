// 无竞争场景下 plain i++ 与 atomic increment 的性能对比。
//
// 三种计数方式：
//   1. volatile plain : 每轮一次 load + add + store，无锁（用 volatile 阻止编译器
//                       把整个循环折叠成一次加法，这是 "i++" 在真实代码里的形态）
//   2. atomic relaxed : __atomic_add_fetch(__ATOMIC_RELAXED)
//                       x86 上仍是 lock xadd；aarch64 上是 ldadd（ARMv8.1 LSE）或 ldxr/stxr 循环
//   3. atomic seq_cst : __atomic_add_fetch(__ATOMIC_SEQ_CST)，语义最强
//
// 用 GCC/Clang 内建而不是 <stdatomic.h>，避免某些环境头文件缺失。
//
// 编译: gcc -O2 -o atomic-inc-bench.out atomic-inc-bench.c
#include <stdio.h>
#include <time.h>

#define ITERATIONS 1000000000L

static volatile long plain_counter;
static long atomic_counter;

static double now_sec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main(void)
{
	double t0, t1;

	t0 = now_sec();
	for (long i = 0; i < ITERATIONS; i++)
		plain_counter++;
	t1 = now_sec();
	printf("volatile plain i++        : %8.3f s  (%6.2f ns/op)  counter=%ld\n",
	       t1 - t0, (t1 - t0) * 1e9 / ITERATIONS, plain_counter);

	atomic_counter = 0;
	t0 = now_sec();
	for (long i = 0; i < ITERATIONS; i++)
		__atomic_add_fetch(&atomic_counter, 1, __ATOMIC_RELAXED);
	t1 = now_sec();
	printf("atomic add relaxed        : %8.3f s  (%6.2f ns/op)  counter=%ld\n",
	       t1 - t0, (t1 - t0) * 1e9 / ITERATIONS, atomic_counter);

	atomic_counter = 0;
	t0 = now_sec();
	for (long i = 0; i < ITERATIONS; i++)
		__atomic_add_fetch(&atomic_counter, 1, __ATOMIC_SEQ_CST);
	t1 = now_sec();
	printf("atomic add seq_cst        : %8.3f s  (%6.2f ns/op)  counter=%ld\n",
	       t1 - t0, (t1 - t0) * 1e9 / ITERATIONS, atomic_counter);

	return 0;
}
