#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

static pid_t gettid_local(void)
{
	return (pid_t)syscall(SYS_gettid);
}

static int pin_current_thread(int cpu)
{
	cpu_set_t mask;

	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);
	return pthread_setaffinity_np(pthread_self(), sizeof(mask), &mask);
}

static void usage(const char *prog)
{
	fprintf(stderr, "usage: %s [runtime_sec] [cpu]\n", prog);
	fprintf(stderr, "example: %s 10 1\n", prog);
}

static void print_observe_cmd(pid_t tid)
{
	printf("watch -n 0.2 \"grep -E 'se.avg|load_avg|runnable_avg|util_avg|util_est' /proc/%d/sched\"\n",
	       tid);
	fflush(stdout);
}

int main(int argc, char **argv)
{
	int runtime_sec = 10;
	int cpu = 0;
	int ret;
	long ncpu;
	struct timespec start;
	struct timespec now;
	long last_report_ms = -1;
	volatile unsigned long counter = 0;
	long elapsed_ms;

	if (argc >= 2)
		runtime_sec = atoi(argv[1]);
	if (argc >= 3)
		cpu = atoi(argv[2]);
	if (runtime_sec <= 0) {
		usage(argv[0]);
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

	ret = pin_current_thread(cpu);
	if (ret != 0) {
		errno = ret;
		perror("pthread_setaffinity_np");
		return 1;
	}

	printf("pid=%d tid=%d cpu=%d runtime=%ds mode=busy\n",
	       getpid(), gettid_local(), cpu, runtime_sec);
	fflush(stdout);
	print_observe_cmd(gettid_local());

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (;;) {
		clock_gettime(CLOCK_MONOTONIC, &now);
		elapsed_ms = (now.tv_sec - start.tv_sec) * 1000L +
			     (now.tv_nsec - start.tv_nsec) / 1000000L;
		if (elapsed_ms / 250 != last_report_ms / 250) {
			printf("progress=%ldms counter=%lu\n", elapsed_ms, counter);
			fflush(stdout);
			last_report_ms = elapsed_ms;
		}
		if (elapsed_ms >= runtime_sec * 1000L)
			break;
		counter++;
	}

	printf("done tid=%d counter=%lu\n", gettid_local(), counter);
	return 0;
}
