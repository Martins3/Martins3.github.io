/*
 * Dekker : memory model
 * <!-- 9200812f-3911-4b0e-b6ae-a9e682c7b9df -->*
 *
 * 经典 StoreLoad 乱序验证
 *
 * x86 TSO 允许的唯一一种乱序就是 StoreLoad:
 * store 还滞留在 store buffer 里, 后面 (不同地址的) load 就先完成了。
 * sb.c 在微架构层面计数这个现象, 本测试展示它的软件后果:
 * 经典的 Dekker 互斥算法, 去掉 "升起自己的 flag 之后、读取对方 flag 之前"
 * 的屏障, 两个线程就会同时进入临界区。
 *
 *   T0: flag[0] = 1; if (flag[1] == 0) 进入临界区
 *   T1: flag[1] = 1; if (flag[0] == 0) 进入临界区
 *
 * 双方的 store 都滞留在 store buffer 时, 都读到对方 flag == 0,
 * 互斥失效。x86 和 ARM 都允许, fence 版本修复。
 */
#include "common.h"

/*
 * flag0 / flag1 / turn 故意放在同一条 cache line:
 * 双方的 store 争夺同一条 line 的 ownership, 滞留在 store buffer 的时间
 * 更长, 更容易触发 (实测 Apple Silicon 上分 line 几乎测不到, 同 line 才明显)。
 */
static volatile int flag0, flag1, turn;
static volatile int in_cs PAD_LINE;

static volatile unsigned long violations;
static volatile unsigned long enters;

static void *worker(void *arg)
{
	int i = (int)(long)arg;
	volatile int *mine = i == 0 ? &flag0 : &flag1;
	volatile int *other = i == 0 ? &flag1 : &flag0;

	while (!should_stop) {
		*mine = 1;
		FENCE(); /* StoreLoad 屏障: 去掉它互斥就会失效 */
		while (*other) {
			if (turn != i) {
				*mine = 0;
				while (turn != i)
					cpu_relax();
				*mine = 1;
				FENCE();
			}
		}

		/*
		 * 临界区: 同一时刻只能有一个人, in_cs 应当恒为 0->1->0。
		 * 注意: 记账代码两侧的屏障必须是无条件的 (dmb), 不能用 FENCE()。
		 * 否则 ARM 上退出方的 in_cs=0 可能排在 flag=0 之后可见,
		 * 或进入方对 in_cs 的 load 被排到检查对方 flag 之前,
		 * 即使互斥真的成立也会误报 (实测 ARM fence 版本误报率 80%)。
		 */
		atomic_thread_fence(memory_order_seq_cst);
		int n = in_cs + 1;
		compiler_barrier();
		in_cs = n;
		if (n != 1)
			violations++;
		enters++;
		compiler_barrier();
		in_cs = n - 1;
		atomic_thread_fence(memory_order_seq_cst);

		turn = 1 - i;
		*mine = 0;
	}
	return NULL;
}

int main(int argc, char **argv)
{
	unsigned long secs = parse_secs(argc, argv);
	pthread_t t0, t1, timer;

	pthread_create(&t0, NULL, worker, (void *)0);
	pthread_create(&t1, NULL, worker, (void *)1);
	pthread_create(&timer, NULL, timer_thread, (void *)secs);

	pthread_join(t0, NULL);
	pthread_join(t1, NULL);
	pthread_join(timer, NULL);

	printf("[dekker-%s] arch=%s: mutual-exclusion broken %lu / %lu enters\n",
	       FENCE_MODE, ARCH_NAME, violations, enters);
	return 0;
}
