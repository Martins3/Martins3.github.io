/*
 * MP (Message Passing), 单调递增版本。
 *
 *   writer: for (i = 1;; i++) { x = i; y = i; }   (x 是数据, y 是 flag)
 *   reader: r1 = y; r2 = x; 检测 r1 > r2
 *
 * 读者先读 flag 再读数据: 看到 flag = i 时, 数据 x 至少也应该是 i
 * (x 先写, y 后写)。如果 r1 > r2, 说明读者看到了新的 flag 却配上了
 * 旧的数据, 写者的两个 store 可见顺序被打乱 (或读者的 load 乱序)。
 *
 * 单调递增版本不需要每轮重置, writer 持续制造 store 流量,
 * 比 "每轮重置 + rendezvous" 的版本更容易在 ARM 上触发。
 *
 * x86 (TSO) 禁止, ARM 允许。
 */
#include "common.h"

static volatile unsigned long x PAD_LINE;
static volatile unsigned long y PAD_LINE;
static volatile unsigned long found;
static volatile unsigned long checks;

static void *writer(void *arg)
{
	(void)arg;
	for (unsigned long i = 1; !should_stop; i++) {
		x = i;
		FENCE();
		y = i;
	}
	return NULL;
}

static void *reader(void *arg)
{
	(void)arg;
	while (!should_stop) {
		unsigned long r1 = y;
		FENCE();
		unsigned long r2 = x;

		if (r1 > r2)
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

	printf("[mp-%s] arch=%s: reorder(flag>data) detected %lu / %lu checks\n",
	       FENCE_MODE, ARCH_NAME, found, checks);
	return 0;
}
