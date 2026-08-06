#ifndef MM_COMMON_H
#define MM_COMMON_H

/*
 * 用户态 memory model litmus 测试的公共部分。
 *
 * 由内核模块 m/concurrent/memory_model.c 和 m/concurrent/mm_ll.c 转换而来,
 * 使用 pthread + C11 stdatomic, Linux (gcc) 和 macOS (clang) 都可以编译。
 *
 * 测试变量一律用 volatile + compiler_barrier(), 禁止编译器重排,
 * 从而只观察 CPU 层面的重排。
 *
 * 每个测试编译两个版本:
 *   - nofence: FENCE() 只是编译器屏障, 允许 CPU 重排
 *   - fence:   FENCE() 是 seq_cst 全屏障, 应当禁止对应的乱序结果
 */

#include <stdatomic.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

/* 等价于内核的 barrier(): 只禁止编译器重排 */
#define compiler_barrier() asm volatile("" ::: "memory")

#ifdef USE_FENCE
/* 等价于内核的 smp_mb() */
#define FENCE() atomic_thread_fence(memory_order_seq_cst)
#define FENCE_MODE "fence"
#else
#define FENCE() compiler_barrier()
#define FENCE_MODE "nofence"
#endif

#if defined(__x86_64__) || defined(__i386__)
#define ARCH_NAME "x86"
#define cpu_relax() asm volatile("pause" ::: "memory")
#elif defined(__aarch64__) || defined(__arm__)
#define ARCH_NAME "arm"
#define cpu_relax() asm volatile("yield" ::: "memory")
#else
#define ARCH_NAME "unknown"
#define cpu_relax() compiler_barrier()
#endif

/*
 * 把测试变量放到不同的 cache line 上。
 * 如果两个变量共享一条 cache line, 两个 store 往往随同一个 line 一起
 * 变得可见, mp / lb 这类测试几乎不可能触发 (Apple Silicon 的 cache line
 * 按 128B 对齐处理, 这里取 256 保险)。
 */
#define PAD_LINE __attribute__((aligned(256)))

static volatile bool should_stop;

static inline unsigned long now_sec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec;
}

/* 运行 secs 秒后设置 should_stop, 等价于内核版本里的 msleep(8000) 线程 */
static void *timer_thread(void *arg)
{
	unsigned long deadline = now_sec() + (unsigned long)arg;
	while (!should_stop && now_sec() < deadline)
		usleep(100 * 1000);
	should_stop = true;
	return NULL;
}

/*
 * 基于 epoch 的两线程 rendezvous, 等价于内核版本里的
 * sem_x / sem_y / sem_end 三个信号量:
 * watcher 每轮重置共享变量, 放行两个 actor, 等两个 actor 完成后检查结果。
 */
struct rendezvous {
	atomic_int go[2];
	atomic_int done[2];
};

static inline void rv_init(struct rendezvous *rv)
{
	atomic_init(&rv->go[0], 0);
	atomic_init(&rv->go[1], 0);
	atomic_init(&rv->done[0], 0);
	atomic_init(&rv->done[1], 0);
}

/* actor: 等待 watcher 放行第 epoch 轮 */
static inline void rv_wait_go(atomic_int *go, int epoch)
{
	while (atomic_load_explicit(go, memory_order_acquire) < epoch)
		cpu_relax();
}

/* actor: 通知 watcher 本轮已完成 */
static inline void rv_signal_done(atomic_int *done, int epoch)
{
	atomic_store_explicit(done, epoch, memory_order_release);
}

/* watcher: 放行两个 actor 进入第 epoch 轮 */
static inline void rv_release(struct rendezvous *rv, int epoch)
{
	atomic_store_explicit(&rv->go[0], epoch, memory_order_release);
	atomic_store_explicit(&rv->go[1], epoch, memory_order_release);
}

/* watcher: 等待两个 actor 完成第 epoch 轮 */
static inline void rv_wait_done(struct rendezvous *rv, int epoch)
{
	while (atomic_load_explicit(&rv->done[0], memory_order_acquire) < epoch ||
	       atomic_load_explicit(&rv->done[1], memory_order_acquire) < epoch)
		cpu_relax();
}

static unsigned long parse_secs(int argc, char **argv)
{
	if (argc > 1) {
		long s = atol(argv[1]);
		if (s > 0)
			return (unsigned long)s;
	}
	return 10;
}

#endif /* MM_COMMON_H */
