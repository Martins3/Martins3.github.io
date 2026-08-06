/*
 * LL, 由 m/concurrent/mm_ll.c (memory_model.c 里的 test_ll_logic) 忠实转换而来。
 * 这是一个反面教材: 它的检测条件测不到 CPU 乱序。
 *
 *   T0: x = 1; y = 1
 *   T1: r1 = x; r2 = y
 *
 * 内核版本检测 r1 == 1 && r2 == 0 (看到先写的 x, 看不到后写的 y),
 * 但这只是 "读者的两次 load 跨过 writer 的两次 store" 的时间窗口,
 * 顺序一致性 (SC) 也允许。反方向 r1 == 0 && r2 == 1 同理:
 * 读者的两个 load 分别落在 writer 两个 store 可见之前和之后即可,
 * 实测 x86 加全屏障照样大量 "测到"。
 *
 * 这个形状 (W: x;y / R: x;y) 不存在对乱序敏感的结果,
 * 因为读者先读的恰好是先写的, 任何异常都可以用时间窗口解释。
 * MP (sb.c 同族) 之所以有效, 是它先读 flag (后写的), 再读 data (先写的),
 * 看到 flag 没看到 data 才是乱序铁证。
 *
 * 保留此文件用于对照: 它在任何架构、加不加 fence 都会 "测到",
 * 而 fence 版本的次数不归零, 恰好说明测到的不是乱序。
 */
#include "common.h"

static volatile int ll_x PAD_LINE;
static volatile int ll_y PAD_LINE;
static int ll_r1, ll_r2;

static struct rendezvous rv;

static void *actor(void *arg)
{
	int id = (int)(long)arg;

	for (int e = 1;; e++) {
		rv_wait_go(&rv.go[id], e);
		if (should_stop) {
			rv_signal_done(&rv.done[id], e);
			return NULL;
		}

		if (id == 0) {
			ll_x = 1;
			FENCE();
			ll_y = 1;
		} else {
			ll_r1 = ll_x;
			FENCE();
			ll_r2 = ll_y;
		}
		rv_signal_done(&rv.done[id], e);
	}
}

int main(int argc, char **argv)
{
	unsigned long secs = parse_secs(argc, argv);
	unsigned long iterations = 0, detected = 0;
	pthread_t t0, t1, timer;

	rv_init(&rv);
	pthread_create(&t0, NULL, actor, (void *)0);
	pthread_create(&t1, NULL, actor, (void *)1);
	pthread_create(&timer, NULL, timer_thread, (void *)secs);

	for (int e = 1; !should_stop; e++) {
		/* The Watcher: 等价于内核版本的 ll_thread2 */
		ll_x = 0;
		ll_y = 0;
		compiler_barrier();

		rv_release(&rv, e);
		rv_wait_done(&rv, e);

		/* 内核版本的检测条件, 作为对照保留 (时间窗口假象, 非乱序) */
		if (ll_r1 == 1 && ll_r2 == 0)
			detected++;
		iterations++;
	}

	printf("[ll-%s] arch=%s: racy-timing(r1=1,r2=0) detected %lu / %lu iterations (NOT a reorder proof)\n",
	       FENCE_MODE, ARCH_NAME, detected, iterations);

	int e = (int)iterations + 1;
	rv_release(&rv, e); /* actor 看到 should_stop 后退出 */
	pthread_join(t0, NULL);
	pthread_join(t1, NULL);
	pthread_join(timer, NULL);
	return 0;
}
