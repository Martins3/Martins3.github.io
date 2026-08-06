/*
 * WR, 由 m/concurrent/memory_model.c 里的 test_failed_logic 转换而来。
 * 参考: https://github.com/smcdef/memory-reordering
 *
 *   writer: x = 1; y = 1; y = 0; x = 0   (循环)
 *   reader: 发现 y == 1 && x == 0
 *
 * 读者读到了 y 的新值(倒数第二个 store),
 * 却读到了 x 的更旧的值(已经被最后的 x = 0 覆盖才对),
 * 说明写者的 store 序列被乱序执行。
 *
 * x86 (TSO) 禁止, ARM 允许。
 */
#include "common.h"

static volatile long x PAD_LINE;
static volatile long y PAD_LINE;
static volatile unsigned long found;
static volatile unsigned long checks;

static void *writer(void *arg)
{
	(void)arg;
	while (!should_stop) {
		x = 1;
		FENCE();
		y = 1;
		FENCE();
		y = 0;
		FENCE();
		x = 0;
		FENCE();
	}
	return NULL;
}

static void *reader(void *arg)
{
	(void)arg;
	while (!should_stop) {
		compiler_barrier();
		if (y == 1 && x == 0)
			found++;
		checks++;
	}
	return NULL;
}

int main(int argc, char **argv)
{
	unsigned long secs = parse_secs(argc, argv);
	pthread_t tw, tr, timer;

	pthread_create(&tw, NULL, writer, NULL);
	pthread_create(&tr, NULL, reader, NULL);
	pthread_create(&timer, NULL, timer_thread, (void *)secs);

	pthread_join(tw, NULL);
	pthread_join(tr, NULL);
	pthread_join(timer, NULL);

	printf("[wr-%s] arch=%s: hits(y=1,x=0) %lu / %lu checks (fence 版本不归零说明是时间窗口假象)\n",
	       FENCE_MODE, ARCH_NAME, found, checks);
	return 0;
}
