#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/sched/types.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

struct thread_arg {
	pthread_mutex_t *lock;
	pthread_cond_t *cond;
	int *setup_done;
	int *setup_failed;
	bool *start;
	int id;
	int policy;
	int value;
	int cpu;
	int runtime_sec;
};

static pid_t gettid_local(void)
{
	return (pid_t)syscall(SYS_gettid);
}

static const char *policy_name(int policy)
{
	switch (policy) {
	case SCHED_FIFO:
		return "SCHED_FIFO";
	case SCHED_RR:
		return "SCHED_RR";
	case SCHED_OTHER:
		return "SCHED_OTHER";
	default:
		return "UNKNOWN";
	}
}

static int parse_policy(const char *name)
{
	if (strcmp(name, "fifo") == 0)
		return SCHED_FIFO;
	if (strcmp(name, "rr") == 0)
		return SCHED_RR;
	if (strcmp(name, "other") == 0)
		return SCHED_OTHER;
	return -1;
}

static int pin_current_thread(int cpu)
{
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	return pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask);
}

static int sched_setattr_local(pid_t pid, const struct sched_attr *attr,
			       unsigned int flags)
{
	return syscall(SYS_sched_setattr, pid, attr, flags);
}

static int get_thread_nice(void)
{
	int raw;

	errno = 0;
	raw = syscall(SYS_getpriority, PRIO_PROCESS, gettid_local());
	if (raw < 0)
		return raw;
	return 20 - raw;
}

static int current_sched_priority(void)
{
	struct sched_param param;

	if (sched_getparam(0, &param) < 0)
		return -1;
	return param.sched_priority;
}

static void notify_setup_result(struct thread_arg *arg, bool failed)
{
	pthread_mutex_lock(arg->lock);
	(*arg->setup_done)++;
	if (failed)
		*arg->setup_failed = 1;
	pthread_cond_broadcast(arg->cond);
	while (!*arg->start && !*arg->setup_failed)
		pthread_cond_wait(arg->cond, arg->lock);
	pthread_mutex_unlock(arg->lock);
}

static bool wait_for_start(struct thread_arg *arg)
{
	bool should_start;

	pthread_mutex_lock(arg->lock);
	while (!*arg->start && !*arg->setup_failed)
		pthread_cond_wait(arg->cond, arg->lock);
	should_start = *arg->start;
	pthread_mutex_unlock(arg->lock);
	return should_start;
}

static void busy_loop_for(int runtime_sec, int id)
{
	struct timespec start;
	struct timespec now;
	long last_report_ms = -1;
	volatile unsigned long counter = 0;

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (;;) {
		clock_gettime(CLOCK_MONOTONIC, &now);
		long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000L +
				  (now.tv_nsec - start.tv_nsec) / 1000000L;
		if (elapsed_ms / 250 != last_report_ms / 250) {
			printf("thread[%d] tid=%d policy=%s sched_prio=%d nice=%d progress=%ldms counter=%lu\n",
			       id, gettid_local(), policy_name(sched_getscheduler(0)),
			       current_sched_priority(), get_thread_nice(),
			       elapsed_ms, counter);
			fflush(stdout);
			last_report_ms = elapsed_ms;
		}
		if (elapsed_ms >= runtime_sec * 1000L)
			break;
		counter++;
	}
}

static int configure_current_thread(struct thread_arg *arg)
{
	struct sched_attr attr;

	memset(&attr, 0, sizeof(attr));
	attr.size = sizeof(attr);
	attr.sched_policy = arg->policy;

	if (arg->policy == SCHED_OTHER) {
		attr.sched_nice = arg->value;
	} else {
		attr.sched_priority = arg->value;
	}

	if (sched_setattr_local(0, &attr, 0) < 0)
		return errno;
	return 0;
}

static void print_ready_line(struct thread_arg *arg)
{
	printf("thread[%d] ready tid=%d cpu=%d policy=%s sched_prio=%d nice=%d\n",
	       arg->id, gettid_local(), arg->cpu, policy_name(sched_getscheduler(0)),
	       current_sched_priority(), get_thread_nice());
	fflush(stdout);
}

static void *thread_fn(void *data)
{
	struct thread_arg *arg = data;
	int ret;

	ret = pin_current_thread(arg->cpu);
	if (ret != 0) {
		errno = ret;
		perror("pthread_setaffinity_np");
		notify_setup_result(arg, true);
		return NULL;
	}

	ret = configure_current_thread(arg);
	if (ret != 0) {
		errno = ret;
		perror("sched_setattr");
		notify_setup_result(arg, true);
		return NULL;
	}

	print_ready_line(arg);
	notify_setup_result(arg, false);
	if (!wait_for_start(arg))
		return NULL;

	busy_loop_for(arg->runtime_sec, arg->id);
	printf("thread[%d] done tid=%d\n", arg->id, gettid_local());
	fflush(stdout);
	return NULL;
}

static void usage(const char *prog)
{
	fprintf(stderr,
		"usage: %s <policy0> <value0> <policy1> <value1> [runtime_sec] [cpu]\n",
		prog);
	fprintf(stderr,
		"value means RT priority for fifo/rr, and nice for other\n");
	fprintf(stderr,
		"example: sudo ./%s fifo 80 rr 70 2 0\n",
		prog);
	fprintf(stderr,
		"example: ./%s other 0 other 5 2 0\n",
		prog);
}

int main(int argc, char **argv)
{
	pthread_t threads[2];
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	struct thread_arg args[2];
	int runtime_sec = 2;
	int cpu = 0;
	int setup_done = 0;
	int setup_failed = 0;
	bool start = false;
	int ret;
	long ncpu;

	if (argc < 5) {
		usage(argv[0]);
		return 1;
	}

	args[0].policy = parse_policy(argv[1]);
	args[0].value = atoi(argv[2]);
	args[1].policy = parse_policy(argv[3]);
	args[1].value = atoi(argv[4]);
	if (argc >= 6)
		runtime_sec = atoi(argv[5]);
	if (argc >= 7)
		cpu = atoi(argv[6]);

	if (args[0].policy < 0 || args[1].policy < 0) {
		fprintf(stderr, "unknown policy\n");
		return 1;
	}

	ncpu = sysconf(_SC_NPROCESSORS_ONLN);
	if (ncpu <= 0) {
		perror("sysconf");
		return 1;
	}
	if (cpu < 0 || cpu >= ncpu) {
		fprintf(stderr, "cpu %d out of range, online cpus=%ld\n", cpu, ncpu);
		return 1;
	}

	printf("pid=%d cpu=%d runtime=%ds\n", getpid(), cpu, runtime_sec);
	printf("thread[0]: %s value=%d\n", policy_name(args[0].policy),
	       args[0].value);
	printf("thread[1]: %s value=%d\n", policy_name(args[1].policy),
	       args[1].value);
	fflush(stdout);

	for (int i = 0; i < 2; i++) {
		args[i].lock = &lock;
		args[i].cond = &cond;
		args[i].setup_done = &setup_done;
		args[i].setup_failed = &setup_failed;
		args[i].start = &start;
		args[i].id = i;
		args[i].cpu = cpu;
		args[i].runtime_sec = runtime_sec;

		ret = pthread_create(&threads[i], NULL, thread_fn, &args[i]);
		if (ret != 0) {
			errno = ret;
			perror("pthread_create");
			return 1;
		}
	}

	pthread_mutex_lock(&lock);
	while (setup_done < 2 && !setup_failed)
		pthread_cond_wait(&cond, &lock);
	if (!setup_failed)
		start = true;
	pthread_cond_broadcast(&cond);
	pthread_mutex_unlock(&lock);

	for (int i = 0; i < 2; i++) {
		ret = pthread_join(threads[i], NULL);
		if (ret != 0) {
			errno = ret;
			perror("pthread_join");
			return 1;
		}
	}

	if (setup_failed)
		return 1;
	return 0;
}
