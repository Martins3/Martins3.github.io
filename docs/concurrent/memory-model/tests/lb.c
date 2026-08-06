/*
 * LB (Load Buffering)
 *
 *   T0: r1 = x; y = 1
 *   T1: r2 = y; x = 1
 *
 * 检测 r1 == 1 && r2 == 1:
 * 双方都是先读后写, 如果 load 和随后的 store 被重排,
 * 就可能互相看到对方 store 的新值。
 *
 * x86 (TSO) 禁止, ARM 允许。
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
			r1 = x;
			FENCE();
			y = 1;
		} else {
			r2 = y;
			FENCE();
			x = 1;
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

		if (r1 == 1 && r2 == 1)
			detected++;
		iterations++;
	}

	printf("[lb-%s] arch=%s: reorder(r1=1,r2=1) detected %lu / %lu iterations\n",
	       FENCE_MODE, ARCH_NAME, detected, iterations);

	int e = (int)iterations + 1;
	rv_release(&rv, e); /* actor 看到 should_stop 后退出 */
	pthread_join(t0, NULL);
	pthread_join(t1, NULL);
	pthread_join(timer, NULL);
	return 0;
}
