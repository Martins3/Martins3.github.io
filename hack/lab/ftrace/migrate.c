
/*
 * rt-migrate-test.c
 *
 * Copyright (C) 2007-2009 Steven Rostedt <srostedt@redhat.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License (not later!)
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#define _GNU_SOURCE
#include <stdio.h>
#ifndef __USE_XOPEN2K
# define __USE_XOPEN2K
#endif
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <stdarg.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <linux/unistd.h>
#include <sys/syscall.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>

#define gettid() syscall(__NR_gettid)

int nr_tasks;
int nr_locks;
int lfd;

static int mark_fd = -1;
static __thread char buff[BUFSIZ+1];

static void setup_ftrace_marker(void)
{
	struct stat st;
	char *files[] = {
		"/sys/kernel/debug/tracing/trace_marker",
		"/debug/tracing/trace_marker",
		"/debugfs/tracing/trace_marker",
	};
	int ret;
	int i;

	for (i = 0; i < (sizeof(files) / sizeof(char *)); i++) {
		ret = stat(files[i], &st);
		if (ret >= 0)
			goto found;
	}
	/* todo, check mounts system */
	return;
found:
	mark_fd = open(files[i], O_WRONLY);
}

static void ftrace_write(const char *fmt, ...)
{
	va_list ap;
	int n;

	if (mark_fd < 0)
		return;

	va_start(ap, fmt);
	n = vsnprintf(buff, BUFSIZ, fmt, ap);
	va_end(ap);

	write(mark_fd, buff, n);
}

#define nano2sec(nan) (nan / 1000000000ULL)
#define nano2ms(nan) (nan / 1000000ULL)
#define nano2usec(nan) (nan / 1000ULL)
#define usec2nano(sec) (sec * 1000ULL)
#define ms2nano(ms) (ms * 1000000ULL)
#define sec2nano(sec) (sec * 1000000000ULL)
#define INTERVAL ms2nano(100ULL)
#define RUN_INTERVAL ms2nano(20ULL)
#define NR_RUNS 50
#define PRIO_START 2

#define PROGRESS_CHARS 70

static unsigned long long interval = INTERVAL;
static unsigned long long run_interval = RUN_INTERVAL;
static int nr_runs = NR_RUNS;
static int prio_start = PRIO_START;
static int check;
static int stop;

static unsigned long long now;
static unsigned long long end;

static pthread_barrier_t start_barrier;
static pthread_barrier_t end_barrier;
static pthread_mutex_t *locks;
static unsigned long long **lock_times;
static unsigned long long *lock_time;
static unsigned long long ***lock_wait_time;

static unsigned long long **intervals;
static unsigned long long **intervals_length;
static unsigned long **intervals_loops;
static long *thread_pids;

static char buffer[BUFSIZ];

static void perr(char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsnprintf(buffer, BUFSIZ, fmt, ap);
	va_end(ap);

	perror(buffer);
	fflush(stderr);
	exit(-1);
}

static void usage(char **argv)
{
	char *arg = argv[0];
	char *p = arg+strlen(arg);

	while (p >= arg && *p != '/')
		p--;
	p++;

	printf("Usage:\n"
	       "%s <options> nr_tasks\n\n"
	       "-p prio --prio  prio        base priority to start RT tasks with (2) \n"
	       "-r time --run-time time     Run time (ms) to busy loop the threads (20)\n"
	       "-s time --sleep-time time   Sleep time (ms) between intervals (100)\n"
	       "-m time --maxerr time       Max allowed error (microsecs)\n"
	       "-l loops --loops loops      Number of iterations to run (50)\n"
	       "-c    --check               Stop if lower prio task is quick than higher (off)\n"
	       "  () above are defaults \n",
		p);
	exit(0);
}

static void parse_options (int argc, char *argv[])
{
	for (;;) {
		int option_index = 0;
		/** Options for getopt */
		static struct option long_options[] = {
			{"prio", required_argument, NULL, 'p'},
			{"run-time", required_argument, NULL, 'r'},
			{"sleep-time", required_argument, NULL, 's'},
			{"maxerr", required_argument, NULL, 'm'},
			{"loops", required_argument, NULL, 'l'},
			{"check", no_argument, NULL, 'c'},
			{"help", no_argument, NULL, '?'},
			{NULL, 0, NULL, 0}
		};
		int c = getopt_long (argc, argv, "p:r:s:m:l:ch",
			long_options, &option_index);
		if (c == -1)
			break;
		switch (c) {
		case 'p': prio_start = atoi(optarg); break;
		case 'r':
			run_interval = atoi(optarg);
			break;
		case 's': interval = atoi(optarg); break;
		case 'l': nr_runs = atoi(optarg); break;
		case 'c': check = 1; break;
		case '?':
		case 'h':
			usage(argv);
			break;
		}
	}

}

static unsigned long long get_time(void)
{
	struct timeval tv;
	unsigned long long time;

	gettimeofday(&tv, NULL);

	time = sec2nano(tv.tv_sec);
	time += usec2nano(tv.tv_usec);

	return time;
}

static unsigned long busy_loop(unsigned long long start_time)
{
	unsigned long long time;
	unsigned long l = 0;

	do {
		l++;
		time = get_time();
	} while ((time - start_time) < run_interval);

	return l;
}

static void release_lock(unsigned long long time, int l, int i, long id)
{
	ftrace_write("thread %ld iter %d, release lock %d\n",
		     id, i, l);
	lock_times[l][i] = time - lock_time[l];
	lock_time[l] = get_time();
	pthread_mutex_unlock(&locks[l]);
}

static void grab_locks(long id, int iter)
{
	unsigned long long try_time;
	unsigned long long time;
	int i;

	for (i = 0; i < nr_locks; i++) {
		/*
		 * Grab the next lock before letting go of the
		 * previous one.
		 */
		try_time = get_time();
		pthread_mutex_lock(&locks[i]);
		time = get_time();
		ftrace_write("thread %ld iter %d, took lock %d\n",
			     id, iter, i);

		lock_wait_time[i][id][iter] = time - try_time;
		if (i)
			release_lock(time, i - 1, iter, id);
		busy_loop(get_time());
	}
	release_lock(time, i - 1, iter, id);
}

void *start_task(void *data)
{
	long id = (long)data;
	unsigned long long start_time;
	struct sched_param param = {
		.sched_priority = id + prio_start,
	};
	int ret;
	int high = 0;
	long pid;
	int i;

	pid = gettid();

	/* Check if we are the highest prio task */
	if (id == nr_tasks-1)
		high = 1;

	ret = sched_setscheduler(0, SCHED_FIFO, &param);
	if (ret < 0 && !id)
		fprintf(stderr, "Warning, can't set priorities\n");

	pthread_barrier_wait(&start_barrier);
	start_time = get_time();
	ftrace_write("Thread %d: started %lld diff %lld\n",
		     pid, start_time, start_time - now);
	for (i = 0; i < nr_runs; i++)
		grab_locks(id, i);
	pthread_barrier_wait(&end_barrier);

	return (void*)pid;
}

static void stop_log(int sig)
{
	stop = 1;
}

static int count_cpus(void)
{
	return sysconf(_SC_NPROCESSORS_ONLN);
}

int main (int argc, char **argv)
{
	pthread_t *threads;
	long i, x;
	int ret;
	int cpus;
	struct timespec intv;
	struct sched_param param;

	parse_options(argc, argv);

	signal(SIGINT, stop_log);

	cpus = count_cpus();

	if (argc >= (optind + 1))
		nr_tasks = atoi(argv[optind]);
	else
		nr_tasks = cpus + 1;

	nr_locks = cpus;

	threads = malloc(sizeof(*threads) * nr_tasks);
	if (!threads)
		perr("malloc");
	memset(threads, 0, sizeof(*threads) * nr_tasks);

	locks = malloc(sizeof(*locks) * nr_locks);
	if (!locks)
		perr("malloc");
	memset(locks, 0, sizeof(*locks) * nr_locks);

	for (i = 0; i < nr_locks; i++) {
		ret = pthread_mutex_init(&locks[i], NULL);
		if (ret < 0)
			perr("pthread_mutex_init");
	}

	lock_time = malloc(sizeof(*lock_time) * nr_locks);
	if (!lock_time)
		perr("malloc");
	memset(lock_time, 0, sizeof(*lock_times) * nr_locks);

	lock_times = malloc(sizeof(*lock_times) * nr_locks);
	if (!lock_times)
		perr("malloc");
	for (i = 0; i < nr_locks; i++) {
		lock_times[i] = malloc(sizeof(**lock_times) * nr_runs);
		if (!lock_times[i])
			perr("malloc");
		memset(lock_times[i], 0, sizeof(**lock_times) * nr_runs);
	}

	lock_wait_time = malloc(sizeof(*lock_wait_time) * nr_locks);
	if (!lock_wait_time)
		perr("malloc");
	memset(lock_wait_time, 0, sizeof(*lock_wait_time) * nr_locks);

	for (i = 0; i < nr_locks; i++) {
		lock_wait_time[i] = malloc(sizeof(**lock_wait_time) * nr_tasks);
		if (!lock_wait_time[i])
			perr("malloc");
		memset(lock_wait_time[i], 0, sizeof(**lock_wait_time) * nr_tasks);

		for (x = 0; x < nr_tasks; x++) {
			lock_wait_time[i][x] = malloc(sizeof(***lock_wait_time) * nr_runs);
			if (!lock_wait_time[i][x])
				perr("malloc");
			memset(lock_wait_time[i][x], 0, sizeof(***lock_wait_time) * nr_runs);
		}
	}

	ret = pthread_barrier_init(&start_barrier, NULL, nr_tasks + 1);
	if (ret < 0)
		perr("pthread_barrier_init");
	ret = pthread_barrier_init(&end_barrier, NULL, nr_tasks + 1);
	if (ret < 0)
		perr("pthread_barrier_init");

	for (i = 0; i < nr_locks; i++) {
		ret = pthread_mutex_init(&locks[i], NULL);
		if (ret < 0)
			perr("pthread_mutex_init");
	}

	intervals = malloc(sizeof(void*) * nr_runs);
	if (!intervals)
		perr("malloc intervals array");

	intervals_length = malloc(sizeof(void*) * nr_runs);
	if (!intervals_length)
		perr("malloc intervals length array");

	intervals_loops = malloc(sizeof(void*) * nr_runs);
	if (!intervals_loops)
		perr("malloc intervals loops array");

	thread_pids = malloc(sizeof(long) * nr_tasks);
	if (!thread_pids)
		perr("malloc thread_pids");

	for (i=0; i < nr_runs; i++) {
		intervals[i] = malloc(sizeof(unsigned long long)*nr_tasks);
		if (!intervals[i])
			perr("malloc intervals");
		memset(intervals[i], 0, sizeof(unsigned long long)*nr_tasks);

		intervals_length[i] = malloc(sizeof(unsigned long long)*nr_tasks);
		if (!intervals_length[i])
			perr("malloc length intervals");
		memset(intervals_length[i], 0, sizeof(unsigned long long)*nr_tasks);

		intervals_loops[i] = malloc(sizeof(unsigned long)*nr_tasks);
		if (!intervals_loops[i])
			perr("malloc loops intervals");
		memset(intervals_loops[i], 0, sizeof(unsigned long)*nr_tasks);
	}

	for (i=0; i < nr_tasks; i++) {
		if (pthread_create(&threads[i], NULL, start_task, (void *)i))
			perr("pthread_create");

	}

	/* up our prio above all tasks */
	memset(&param, 0, sizeof(param));
	param.sched_priority = nr_tasks + prio_start;
	if (sched_setscheduler(0, SCHED_FIFO, &param))
		fprintf(stderr, "Warning, can't set priority of main thread!\n");
		


	intv.tv_sec = nano2sec(INTERVAL);
	intv.tv_nsec = INTERVAL % sec2nano(1);

	setup_ftrace_marker();

	pthread_barrier_wait(&start_barrier);
	ftrace_write("All running!!!\n");

	end = get_time();
	ftrace_write("End=%lld now=%lld diff=%lld\n", end, end - now);

	pthread_barrier_wait(&end_barrier);
	putc('\n', stderr);

	for (i=0; i < nr_tasks; i++)
		pthread_join(threads[i], (void*)&thread_pids[i]);

	if (stop) {
		/*
		 * We use this test in bash while loops
		 * So if we hit Ctrl-C then let the while
		 * loop know to break.
		 */
		if (check < 0)
			exit(-1);
		else
			exit(1);
	}
	if (check < 0)
		exit(-1);
	else
		exit(0);

	return 0;
}
