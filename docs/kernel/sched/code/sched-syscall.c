#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/resource.h>
#include <errno.h>
/*
 * 这三个 header 需要仔细阅读下，其实内容不多
 */
#include <linux/sched.h>
#include <sched.h>
#include <linux/sched/types.h>

#include <sys/syscall.h>
#include <sys/types.h>

static int sched_setattr(pid_t pid, const struct sched_attr *attr,
			 unsigned int flags)
{
	return syscall(SYS_sched_setattr, pid, attr, flags);
}

static int sched_getattr(pid_t pid, const struct sched_attr *attr,
			 unsigned int size, unsigned int flags)
{
	return syscall(SYS_sched_getattr, pid, attr, size, flags);
}

#ifdef TODO
static void show_priority_range()
{
	int x = sched_get_priority_max(SCHED_FIFO);
	int sched_get_priority_min(int policy);
}
#endif
static char sched_name[9][32] = { "SCHED_NORMAL",   "SCHED_FIFO",
				  "SCHED_RR",	    "SCHED_BATCH",
				  "SCHED_ISO",	    "SCHED_IDLE",
				  "SCHED_DEADLINE", "SCHED_EXT",
				  "SCHED_MARTINS3" };
static inline const char *get_scheduler_name(int policy)
{
	return sched_name[policy];
}

int test_sched_getscheduler()
{
	pid_t pid = getpid(); // Get the process ID of the current process

	// Get the scheduling policy of the current process
	int policy = sched_getscheduler(pid);
	if (policy == -1) {
		perror("sched_getscheduler");
		return 1;
	}
	printf("scheduler is %s", get_scheduler_name(policy));
	return 0;
}

int test_sched_setscheduler()
{
	// Get the process ID of the current process
	pid_t pid = getpid();

	// Define the scheduling parameters
	struct sched_param param;
	// TODO 如果我的 scheduler 如何利用这个?
	param.sched_priority = 0;

	// Set the scheduling policy to SCHED_RR (Round Robin)
	int policy = SCHED_RR;
	int result = sched_setscheduler(pid, policy, &param);

	// Check for errors
	if (result == -1) {
		perror("sched_setscheduler failed");
		return -1;
	}

	// Retrieve the new scheduling policy to verify the change
	int new_policy = sched_getscheduler(pid);
	if (new_policy == -1) {
		perror("sched_getscheduler failed");
		return -1;
	}

	// Print the new scheduling policy
	printf("Scheduling policy for PID %d changed to: ", pid);
	return 0;
}

/*
 * nice 就是 getpriority 和 setpriority 的封装
 */
int test_nice()
{
	int current_nice = nice(0); // 获取当前 nice 值
	if (current_nice == -1 && errno != 0) {
		perror("nice");
		return 1;
	}
	printf("Current nice value: %d\n", current_nice);

	int new_nice = nice(5);
	if (new_nice == -1 && errno != 0) {
		perror("nice");
		return 1;
	}
	printf("New nice value after increasing by 5: %d\n", new_nice);

	new_nice = nice(-5); // 减少 5
	if (new_nice == -1 && errno != 0) {
		if (errno == EPERM) {
			printf("Permission denied: Only superuser can decrease nice value.\n");
		} else {
			perror("nice");
			return 1;
		}
	} else {
		printf("New nice value after decreasing by 5: %d\n", new_nice);
	}

	return 0;
}

/*
 * 看来其他的
 * SCHED_NORMAL [0, 0]
 * SCHED_FIFO [1, 99]
 * SCHED_RR [1, 99]
 * SCHED_BATCH [0, 0]
 * SCHED_IDLE [0, 0]
 * SCHED_DEADLINE [0, 0]
 * SCHED_EXT [0, 0]
 */
int test_sched_get_priority_max(void)
{
	int max_priority;
	int min_priority;
	for (size_t policy = 0; policy < 8; policy++) {
		max_priority = sched_get_priority_max(policy);
		if (max_priority == -1) {
			/* perror("sched_get_priority_max"); */
			continue;
		}

		min_priority = sched_get_priority_min(policy);
		if (min_priority == -1) {
			perror("sched_get_priority_min");
			return 1;
		}
		printf("%s [%d, %d]\n", get_scheduler_name(policy),
		       min_priority, max_priority);
	}
	return 0;
}

/*
 * getpriority 和 setpriority 都是定义在 kernel/sys.c 中的
 * 猜测这个是从 unix 时代继承的东西，而 sched_get_priority_max
 */
int test_setpriority()
{
	int which = PRIO_PROCESS; // Set priority for a specific process
	id_t who = getpid(); // Use the current process ID
	int priority = -1; // Desired priority (niceness value)

	// Set the priority
	int result = setpriority(which, who, priority);

	if (result == -1) {
		// Error handling
		perror("setpriority");
		return 1;
	}

	// Verify the priority
	errno = 0;
	int current_priority = getpriority(which, who);
	if (errno != 0) {
		perror("getpriority");
		return 1;
	}

	printf("Priority set successfully. Current priority: %d\n",
	       current_priority);

	return 0;
}

/*
 * sched_getattr 是新提供的 syscall ，更加复杂，也更加通用的
 */
//           struct sched_attr {
//               u32 size;              /* Size of this structure */
//               u32 sched_policy;      /* Policy (SCHED_*) */
//               u64 sched_flags;       /* Flags */
//               s32 sched_nice;        /* Nice value (SCHED_OTHER,
//                                         SCHED_BATCH) */
//               u32 sched_priority;    /* Static priority (SCHED_FIFO,
//                                         SCHED_RR) */
//               /* For SCHED_DEADLINE */
//               u64 sched_runtime;
//               u64 sched_deadline;
//               u64 sched_period;
//
//               /* Utilization hints */
//               u32 sched_util_min;
//               u32 sched_util_max;
//           };
//
int test_sched_setattr()
{
	struct sched_attr attr;
	int res;
	memset(&attr, 0, sizeof(struct sched_attr));
	attr.size = sizeof(struct sched_attr);

	res = sched_getattr(getpid(), &attr, attr.size, 0);
	printf("[martins3:%s:%d] %d nice=%d %llx\n", __FUNCTION__, __LINE__,
	       attr.sched_policy, attr.sched_nice, attr.sched_flags);

	attr.sched_nice += 1;
	res = sched_setattr(getpid(), &attr, 0);
	if (res < 0) {
		perror("sched_setattr");
		return 1;
	}
	// 这里展示一个例子，重新获取的
	int nice = getpriority(PRIO_PROCESS, getpid());
	if (nice == -1) {
		perror("getpriority");
		return 1;
	}
	printf("[martins3:%s:%d] %d\n", __FUNCTION__, __LINE__, nice);
	return 0;
}

int test_sched_rr_get_interval()
{
	struct timespec interval;
	pid_t pid = getpid();
	int ret = sched_rr_get_interval(pid, &interval);

	if (ret == -1) {
		perror("sched_rr_get_interval failed");
		return 1;
	}

	printf("SCHED_RR time slice for PID %d: %ld seconds, %ld nanoseconds\n",
	       pid, interval.tv_sec, interval.tv_nsec);

	return 0;
}

/*
 * TODO sched_setaffinity
 */

int main(int argc, char *argv[])
{
	/* test_attr(); */
	/* test_sched(); */
	/* test_sched_setattr(); */
	/* test_nice(); */
	/* test_sched_get_priority_max(); */
	/* test_setpriority(); */
	test_sched_rr_get_interval();
	/*
	 * 如果是使用 martins3 scheduler 可以调用到这两个
	 *
	 * balance_martins3
	 * pick_next_task_martins3
	 */
	for (size_t i = 0; i < 1000; i++) {
		sleep(1);
	}

	return 0;
}
