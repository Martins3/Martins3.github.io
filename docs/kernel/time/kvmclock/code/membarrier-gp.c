/*
 * membarrier-gp.c: 让当前进程反复阻塞在一个 RCU grace period 上。
 *
 * MEMBARRIER_CMD_GLOBAL 在内核中就是 synchronize_rcu()，而且不需要任何权限。
 * 注意 kernel/sched/membarrier.c 中的判断:
 *
 *	case MEMBARRIER_CMD_GLOBAL:
 *		if (tick_nohz_full_enabled())
 *			return -EINVAL;
 *		if (num_online_cpus() > 1)
 *			synchronize_rcu();
 *		return 0;
 *
 * 所以 guest 只有 1 个 online CPU 的时候该 syscall 是空操作，
 * 必须在 2 个以上 vCPU 的 guest 中才有意义。
 *
 * gcc -O2 -o membarrier-gp.out membarrier-gp.c
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>
#include <unistd.h>

#define MEMBARRIER_CMD_QUERY 0
#define MEMBARRIER_CMD_GLOBAL 1
#define MEMBARRIER_CMD_GLOBAL_EXPEDITED 2
#define MEMBARRIER_CMD_REGISTER_GLOBAL_EXPEDITED 4

static long mb(unsigned int cmd)
{
	return syscall(SYS_membarrier, cmd, 0);
}

int main(int argc, char **argv)
{
	unsigned int cmd = MEMBARRIER_CMD_GLOBAL;
	long q;

	q = mb(MEMBARRIER_CMD_QUERY);
	printf("MEMBARRIER_CMD_QUERY=%#lx mode=%s\n", q, argc > 1 ? argv[1] : "global");
	fflush(stdout);

	if (argc > 1 && strcmp(argv[1], "expedited") == 0) {
		if (!(q & MEMBARRIER_CMD_REGISTER_GLOBAL_EXPEDITED)) {
			printf("expedited not supported\n");
			return 1;
		}
		if (mb(MEMBARRIER_CMD_REGISTER_GLOBAL_EXPEDITED) != 0) {
			perror("MEMBARRIER_CMD_REGISTER_GLOBAL_EXPEDITED");
			return 1;
		}
		cmd = MEMBARRIER_CMD_GLOBAL_EXPEDITED;
	}

	for (;;) {
		mb(cmd);
	}
}
