#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
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
	int cpu;
	int runtime_sec;
};

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

static void print_observe_cmd(int id, pid_t tid)
{
	printf("thread[%d] observe:\n watch -n 0.2 \"grep -E 'se.avg|load_avg|runnable_avg|util_avg|util_est' /proc/%d/sched\"\n",
	       id, tid);
	fflush(stdout);
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

static unsigned long busy_loop_for(int runtime_sec, int id)
{
	struct timespec start;
	struct timespec now;
	long last_report_ms = -1;
	volatile unsigned long counter = 0;
	long elapsed_ms;

	clock_gettime(CLOCK_MONOTONIC, &start);

	for (;;) {
		clock_gettime(CLOCK_MONOTONIC, &now);
		elapsed_ms = (now.tv_sec - start.tv_sec) * 1000L +
			     (now.tv_nsec - start.tv_nsec) / 1000000L;
		if (elapsed_ms / 250 != last_report_ms / 250) {
			printf("thread[%d] tid=%d progress=%ldms counter=%lu\n",
			       id, gettid_local(), elapsed_ms, counter);
			fflush(stdout);
			last_report_ms = elapsed_ms;
		}
		if (elapsed_ms >= runtime_sec * 1000L)
			break;
		counter++;
	}

	return counter;
}

static void *thread_fn(void *data)
{
	struct thread_arg *arg = data;
	unsigned long counter;
	int ret;

	ret = pin_current_thread(arg->cpu);
	if (ret != 0) {
		errno = ret;
		perror("pthread_setaffinity_np");
		notify_setup_result(arg, true);
		return NULL;
	}

	printf("thread[%d] ready tid=%d cpu=%d policy=SCHED_OTHER\n", arg->id,
	       gettid_local(), arg->cpu);
	fflush(stdout);
	print_observe_cmd(arg->id, gettid_local());

	notify_setup_result(arg, false);
	if (!wait_for_start(arg))
		return NULL;

	counter = busy_loop_for(arg->runtime_sec, arg->id);
	printf("thread[%d] done tid=%d counter=%lu\n", arg->id, gettid_local(),
	       counter);
	fflush(stdout);
	return NULL;
}

static void usage(const char *prog)
{
	fprintf(stderr, "usage: %s [runtime_sec] [cpu]\n", prog);
	fprintf(stderr, "example: %s 10 1\n", prog);
}

int main(int argc, char **argv)
{
	pthread_t threads[2];
	pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
	struct thread_arg args[2];
	int runtime_sec = 10;
	int cpu = 0;
	int setup_done = 0;
	int setup_failed = 0;
	bool start = false;
	long ncpu;
	int i;

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
		fprintf(stderr, "cpu %d out of range, online cpus=%ld\n", cpu,
			ncpu);
		return 1;
	}

	printf("pid=%d cpu=%d runtime=%ds mode=contention\n", getpid(), cpu,
	       runtime_sec);

	for (i = 0; i < 2; i++) {
		args[i].lock = &lock;
		args[i].cond = &cond;
		args[i].setup_done = &setup_done;
		args[i].setup_failed = &setup_failed;
		args[i].start = &start;
		args[i].id = i;
		args[i].cpu = cpu;
		args[i].runtime_sec = runtime_sec;

		if (pthread_create(&threads[i], NULL, thread_fn, &args[i]) !=
		    0) {
			perror("pthread_create");
			return 1;
		}
	}

	pthread_mutex_lock(&lock);
	while (setup_done < 2 && !setup_failed)
		pthread_cond_wait(&cond, &lock);
	if (!setup_failed) {
		start = true;
		pthread_cond_broadcast(&cond);
	}
	pthread_mutex_unlock(&lock);

	for (i = 0; i < 2; i++)
		pthread_join(threads[i], NULL);

	return setup_failed ? 1 : 0;
}
