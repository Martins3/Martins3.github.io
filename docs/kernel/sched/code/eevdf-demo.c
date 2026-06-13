#define _GNU_SOURCE

#include <errno.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>

struct shared_state {
	pthread_mutex_t lock;
	pthread_cond_t cond;
	int setup_done;
	int setup_failed;
	bool start;
	bool stop;
};

struct latency_stats {
	long *samples_us;
	size_t nr_samples;
	size_t max_samples;
	long min_us;
	long max_us;
	long long sum_us;
	long late_over_100us;
	long late_over_500us;
	long late_over_1000us;
};

struct thread_arg {
	struct shared_state *shared;
	int id;
	int cpu;
	int nice;
	int runtime_sec;
	int period_us;
	int work_us;
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

static int set_thread_nice(int nice)
{
	return syscall(SYS_setpriority, PRIO_PROCESS, gettid_local(), nice);
}

static void print_sched_watch(const char *label, pid_t tid)
{
	printf("%s observe: watch -n 0.2 \"grep -E 'se\\.|vruntime|deadline|slice|sum_exec_runtime|nr_switches|nr_voluntary_switches|nr_involuntary_switches' /proc/%d/sched\"\n",
	       label, tid);
	fflush(stdout);
}

static long diff_us(const struct timespec *a, const struct timespec *b)
{
	return (a->tv_sec - b->tv_sec) * 1000000L +
	       (a->tv_nsec - b->tv_nsec) / 1000L;
}

static struct timespec add_us(struct timespec t, long us)
{
	t.tv_nsec += us * 1000L;
	while (t.tv_nsec >= 1000000000L) {
		t.tv_sec++;
		t.tv_nsec -= 1000000000L;
	}
	return t;
}

static void busy_for_us(int work_us)
{
	struct timespec start;
	struct timespec now;
	volatile unsigned long counter = 0;

	clock_gettime(CLOCK_MONOTONIC, &start);
	for (;;) {
		clock_gettime(CLOCK_MONOTONIC, &now);
		if (diff_us(&now, &start) >= work_us)
			break;
		counter++;
	}
}

static void notify_setup_result(struct shared_state *shared, bool failed)
{
	pthread_mutex_lock(&shared->lock);
	shared->setup_done++;
	if (failed)
		shared->setup_failed = 1;
	pthread_cond_broadcast(&shared->cond);
	while (!shared->start && !shared->setup_failed)
		pthread_cond_wait(&shared->cond, &shared->lock);
	pthread_mutex_unlock(&shared->lock);
}

static bool wait_for_start(struct shared_state *shared)
{
	bool should_start;

	pthread_mutex_lock(&shared->lock);
	while (!shared->start && !shared->setup_failed)
		pthread_cond_wait(&shared->cond, &shared->lock);
	should_start = shared->start;
	pthread_mutex_unlock(&shared->lock);
	return should_start;
}

static int cmp_long(const void *a, const void *b)
{
	long lhs = *(const long *)a;
	long rhs = *(const long *)b;

	if (lhs < rhs)
		return -1;
	if (lhs > rhs)
		return 1;
	return 0;
}

static long percentile_us(struct latency_stats *stats, int pct)
{
	size_t idx;

	if (!stats->nr_samples)
		return 0;

	idx = (stats->nr_samples - 1) * (size_t)pct / 100;
	return stats->samples_us[idx];
}

static void print_latency_summary(struct latency_stats *stats, int period_us, int work_us)
{
	long p50_us;
	long p95_us;
	long p99_us;
	double avg_us;

	if (!stats->nr_samples)
		return;

	qsort(stats->samples_us, stats->nr_samples, sizeof(stats->samples_us[0]), cmp_long);
	p50_us = percentile_us(stats, 50);
	p95_us = percentile_us(stats, 95);
	p99_us = percentile_us(stats, 99);
	avg_us = (double)stats->sum_us / (double)stats->nr_samples;

	printf("\nsummary:\n");
	printf("  periodic thread: samples=%zu period=%dus work=%dus\n",
	       stats->nr_samples, period_us, work_us);
	printf("  wake latency us: min=%ld avg=%.1f p50=%ld p95=%ld p99=%ld max=%ld\n",
	       stats->min_us, avg_us, p50_us, p95_us, p99_us, stats->max_us);
	printf("  wake latency buckets: >100us=%ld >500us=%ld >1000us=%ld\n",
	       stats->late_over_100us, stats->late_over_500us, stats->late_over_1000us);
	fflush(stdout);
}

static void *hog_thread_fn(void *data)
{
	struct thread_arg *arg = data;
	struct timespec start;
	struct timespec now;
	long last_report_ms = -1;
	volatile unsigned long counter = 0;
	int ret;
	pid_t tid;

	ret = pin_current_thread(arg->cpu);
	if (ret != 0) {
		errno = ret;
		perror("pthread_setaffinity_np");
		notify_setup_result(arg->shared, true);
		return NULL;
	}

	if (set_thread_nice(arg->nice) < 0) {
		perror("setpriority");
		notify_setup_result(arg->shared, true);
		return NULL;
	}

	tid = gettid_local();
	printf("thread[0] role=hog tid=%d cpu=%d nice=%d\n", tid, arg->cpu, arg->nice);
	print_sched_watch("thread[0]", tid);

	notify_setup_result(arg->shared, false);
	if (!wait_for_start(arg->shared))
		return NULL;

	clock_gettime(CLOCK_MONOTONIC, &start);
	while (!__atomic_load_n(&arg->shared->stop, __ATOMIC_RELAXED)) {
		clock_gettime(CLOCK_MONOTONIC, &now);
		long elapsed_ms = (now.tv_sec - start.tv_sec) * 1000L +
				  (now.tv_nsec - start.tv_nsec) / 1000000L;
		if (elapsed_ms / 500 != last_report_ms / 500) {
			printf("thread[0] progress=%ldms counter=%lu\n", elapsed_ms, counter);
			fflush(stdout);
			last_report_ms = elapsed_ms;
		}
		counter++;
	}

	printf("thread[0] done tid=%d counter=%lu\n", tid, counter);
	fflush(stdout);
	return NULL;
}

static void *periodic_thread_fn(void *data)
{
	struct thread_arg *arg = data;
	struct latency_stats stats = { 0 };
	struct timespec start;
	struct timespec deadline;
	struct timespec wake;
	struct timespec cpu_before;
	struct timespec cpu_after;
	long last_report_iter = -1;
	long iterations;
	long i;
	int ret;
	pid_t tid;

	ret = pin_current_thread(arg->cpu);
	if (ret != 0) {
		errno = ret;
		perror("pthread_setaffinity_np");
		notify_setup_result(arg->shared, true);
		return NULL;
	}

	if (set_thread_nice(arg->nice) < 0) {
		perror("setpriority");
		notify_setup_result(arg->shared, true);
		return NULL;
	}

	iterations = (long)(((long long)arg->runtime_sec * 1000000LL) / arg->period_us);
	if (iterations < 1)
		iterations = 1;

	stats.samples_us = calloc((size_t)iterations, sizeof(long));
	if (!stats.samples_us) {
		perror("calloc");
		notify_setup_result(arg->shared, true);
		return NULL;
	}
	stats.max_samples = (size_t)iterations;
	stats.min_us = 1L << 30;

	tid = gettid_local();
	printf("thread[1] role=periodic tid=%d cpu=%d nice=%d period=%dus work=%dus iterations=%ld\n",
	       tid, arg->cpu, arg->nice, arg->period_us, arg->work_us, iterations);
	print_sched_watch("thread[1]", tid);

	notify_setup_result(arg->shared, false);
	if (!wait_for_start(arg->shared)) {
		free(stats.samples_us);
		return NULL;
	}

	clock_gettime(CLOCK_MONOTONIC, &start);
	deadline = add_us(start, arg->period_us);

	for (i = 0; i < iterations; i++) {
		if (clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &deadline, NULL) != 0)
			break;

		clock_gettime(CLOCK_MONOTONIC, &wake);
		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cpu_before);

		long wake_latency_us = diff_us(&wake, &deadline);
		if (wake_latency_us < 0)
			wake_latency_us = 0;

		if (stats.nr_samples < stats.max_samples)
			stats.samples_us[stats.nr_samples++] = wake_latency_us;
		stats.sum_us += wake_latency_us;
		if (wake_latency_us < stats.min_us)
			stats.min_us = wake_latency_us;
		if (wake_latency_us > stats.max_us)
			stats.max_us = wake_latency_us;
		if (wake_latency_us > 100)
			stats.late_over_100us++;
		if (wake_latency_us > 500)
			stats.late_over_500us++;
		if (wake_latency_us > 1000)
			stats.late_over_1000us++;

		busy_for_us(arg->work_us);

		clock_gettime(CLOCK_THREAD_CPUTIME_ID, &cpu_after);

		if (i / 10 != last_report_iter / 10) {
			long cpu_runtime_us = diff_us(&cpu_after, &cpu_before);
			printf("thread[1] iter=%ld wake_latency=%ldus cpu_runtime=%ldus\n",
			       i, wake_latency_us, cpu_runtime_us);
			fflush(stdout);
			last_report_iter = i;
		}

		deadline = add_us(deadline, arg->period_us);
	}

	__atomic_store_n(&arg->shared->stop, true, __ATOMIC_RELAXED);
	print_latency_summary(&stats, arg->period_us, arg->work_us);
	printf("thread[1] done tid=%d\n", tid);
	fflush(stdout);
	free(stats.samples_us);
	return NULL;
}

static void usage(const char *prog)
{
	fprintf(stderr,
		"usage: %s [runtime_sec] [cpu] [period_us] [work_us] [hog_nice] [periodic_nice]\n",
		prog);
	fprintf(stderr,
		"example: %s 10 1 20000 500 0 0\n",
		prog);
	fprintf(stderr,
		"meaning: one CPU hog + one periodic short task on the same CPU\n");
}

int main(int argc, char **argv)
{
	pthread_t hog_thread;
	pthread_t periodic_thread;
	struct shared_state shared = {
		.lock = PTHREAD_MUTEX_INITIALIZER,
		.cond = PTHREAD_COND_INITIALIZER,
	};
	struct thread_arg hog_arg = { 0 };
	struct thread_arg periodic_arg = { 0 };
	int runtime_sec = 10;
	int cpu = 0;
	int period_us = 20000;
	int work_us = 500;
	int hog_nice = 0;
	int periodic_nice = 0;
	long ncpu;

	if (argc >= 2)
		runtime_sec = atoi(argv[1]);
	if (argc >= 3)
		cpu = atoi(argv[2]);
	if (argc >= 4)
		period_us = atoi(argv[3]);
	if (argc >= 5)
		work_us = atoi(argv[4]);
	if (argc >= 6)
		hog_nice = atoi(argv[5]);
	if (argc >= 7)
		periodic_nice = atoi(argv[6]);

	if (runtime_sec <= 0 || period_us <= 0 || work_us < 0) {
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

	printf("pid=%d cpu=%d runtime=%ds period=%dus work=%dus hog_nice=%d periodic_nice=%d\n",
	       getpid(), cpu, runtime_sec, period_us, work_us, hog_nice, periodic_nice);
	printf("goal: observe how a short periodic SCHED_OTHER task wakes up against a CPU hog\n");
	fflush(stdout);

	hog_arg.shared = &shared;
	hog_arg.id = 0;
	hog_arg.cpu = cpu;
	hog_arg.nice = hog_nice;
	hog_arg.runtime_sec = runtime_sec;

	periodic_arg.shared = &shared;
	periodic_arg.id = 1;
	periodic_arg.cpu = cpu;
	periodic_arg.nice = periodic_nice;
	periodic_arg.runtime_sec = runtime_sec;
	periodic_arg.period_us = period_us;
	periodic_arg.work_us = work_us;

	if (pthread_create(&hog_thread, NULL, hog_thread_fn, &hog_arg) != 0) {
		perror("pthread_create hog");
		return 1;
	}

	if (pthread_create(&periodic_thread, NULL, periodic_thread_fn, &periodic_arg) != 0) {
		perror("pthread_create periodic");
		return 1;
	}

	pthread_mutex_lock(&shared.lock);
	while (shared.setup_done < 2 && !shared.setup_failed)
		pthread_cond_wait(&shared.cond, &shared.lock);
	if (!shared.setup_failed) {
		shared.start = true;
		pthread_cond_broadcast(&shared.cond);
	}
	pthread_mutex_unlock(&shared.lock);

	pthread_join(periodic_thread, NULL);
	__atomic_store_n(&shared.stop, true, __ATOMIC_RELAXED);
	pthread_join(hog_thread, NULL);

	return shared.setup_failed ? 1 : 0;
}
