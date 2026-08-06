/*
 * SS, 由 m/concurrent/memory_model.c 里的 test_ss_logic 转换而来。
 *
 *   T0: count++                          (单调递增的计数器)
 *   T1: temp = count; a = temp; b = temp (把同一个快照写进 a 和 b)
 *   T2: d = b; c = a; 检测 d - c > 0
 *
 * 因为 a 和 b 被写入的是同一个 temp, 理论上任意时刻 a == b。
 * 如果 T2 观察到 d > c, 说明 T1 的 b = temp 比 a = temp 先对其他 CPU
 * 可见 (store 乱序), 且 T2 恰好在两次更新之间读到了 "新 b 旧 a"。
 *
 * x86 (TSO) 禁止, ARM 允许。
 */
#include "common.h"

static volatile unsigned int count PAD_LINE;
static volatile unsigned int a PAD_LINE;
static volatile unsigned int b PAD_LINE;
static volatile unsigned long found;
static volatile unsigned long checks;

static void *ss_thread0(void *arg)
{
	(void)arg;
	while (!should_stop)
		count++;
	return NULL;
}

static void *ss_thread1(void *arg)
{
	(void)arg;
	while (!should_stop) {
		unsigned int temp = count;

		a = temp;
		FENCE();
		b = temp;
	}
	return NULL;
}

static void *ss_thread2(void *arg)
{
	(void)arg;
	while (!should_stop) {
		unsigned int c, d;

		d = b;
		FENCE();
		c = a;

		if ((int)(d - c) > 0)
			found++;
		checks++;
	}
	return NULL;
}

int main(int argc, char **argv)
{
	unsigned long secs = parse_secs(argc, argv);
	pthread_t t0, t1, t2, timer;

	pthread_create(&t0, NULL, ss_thread0, NULL);
	pthread_create(&t1, NULL, ss_thread1, NULL);
	pthread_create(&t2, NULL, ss_thread2, NULL);
	pthread_create(&timer, NULL, timer_thread, (void *)secs);

	pthread_join(t0, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	pthread_join(timer, NULL);

	printf("[ss-%s] arch=%s: reorder(d>c) detected %lu / %lu checks\n",
	       FENCE_MODE, ARCH_NAME, found, checks);
	return 0;
}
