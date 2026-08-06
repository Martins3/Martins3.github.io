/*
 * SB (Store Buffering)
 *
 *   T0: x = 1; r1 = y
 *   T1: y = 1; r2 = x
 *
 * 检测 r1 == 0 && r2 == 0:
 * 双方都先写后读, 如果写操作还滞留在 store buffer 里,
 * 两个读就都可能拿到旧值 0。
 *
 * x86 和 ARM 都允许, 是最容易触发的效果。
 * 加 FENCE 后两边都不允许。
 */
#include "common.h"

static volatile int x PAD_LINE;
static volatile int y PAD_LINE;
static int r1, r2;

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
			x = 1;
			FENCE();
			r1 = y;
		} else {
			y = 1;
			FENCE();
			r2 = x;
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
		x = 0;
		y = 0;
		compiler_barrier();

		rv_release(&rv, e);
		rv_wait_done(&rv, e);

		if (r1 == 0 && r2 == 0)
			detected++;
		iterations++;
	}

	printf("[sb-%s] arch=%s: reorder(r1=0,r2=0) detected %lu / %lu iterations\n",
	       FENCE_MODE, ARCH_NAME, detected, iterations);

	int e = (int)iterations + 1;
	rv_release(&rv, e); /* actor 看到 should_stop 后退出 */
	pthread_join(t0, NULL);
	pthread_join(t1, NULL);
	pthread_join(timer, NULL);
	return 0;
}
