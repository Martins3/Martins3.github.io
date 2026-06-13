/*
 * after_demo_locked.c: Fix "time going backwards" with pthread_mutex.
 *
 * Based on perfbook appendix/questions/after.tex (CodeSamples/api-pthreads/QAfter/timelocked.c)
 *
 * Adding a mutex around the snapshot operations eliminates the anomaly
 * because the critical sections are guaranteed to execute atomically
 * with respect to each other.
 *
 * Build: make after_demo_locked.out
 * Run:   ./after_demo_locked.out
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <poll.h>
#include <sys/time.h>
#include <unistd.h>

static double dgettimeofday(void)
{
	struct timeval tv;
	if (gettimeofday(&tv, NULL) != 0) {
		perror("gettimeofday");
		exit(-1);
	}
	return tv.tv_sec + ((double)tv.tv_usec) / 1000000.;
}

static long maxdelta = 1000;
#define NWATCHERS 4

static pthread_mutex_t snaplock = PTHREAD_MUTEX_INITIALIZER;

static int modgreater(long a, long b)
{
	return (a - b) > 0;
}

struct snapshot {
	double t;
	long a;
	long b;
	long c;
} ss = { 0.0, 0, 0, 0 };

static volatile int goflag = 0;
static volatile int producer_ready = 0;
static volatile int producer_done = 0;

static void *producer(void *ignored)
{
	int en;
	int i = 0;

	producer_ready = 1;
	while (!goflag)
		sched_yield();
	while (goflag) {
		en = pthread_mutex_lock(&snaplock);
		if (en != 0) {
			fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(en));
			exit(EXIT_FAILURE);
		}
		ss.t = dgettimeofday();
		ss.a = ss.c + 1;
		ss.b = ss.a + 1;
		ss.c = ss.b + 1;
		en = pthread_mutex_unlock(&snaplock);
		if (en != 0) {
			fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(en));
			exit(EXIT_FAILURE);
		}
		i++;
	}
	printf("producer exiting: %d samples\n", i);
	producer_done = 1;
	return NULL;
}

#define NSNAPS 100

struct snapshot_consumer {
	double t;
	double tc;
	long a;
	long b;
	long c;
	long sequence;
	int iserror;
} ssc[NSNAPS];

static int curseq = 0;
static volatile int consumer_ready = 0;
static volatile int consumer_done = 0;

static void *consumer(void *ignored)
{
	struct snapshot_consumer curssc;
	int en;
	int i = 0;
	int j = 0;

	consumer_ready = 1;
	while (ss.t == 0.0)
		sched_yield();
	while (goflag) {
		en = pthread_mutex_lock(&snaplock);
		if (en != 0) {
			fprintf(stderr, "pthread_mutex_lock: %s\n", strerror(en));
			exit(EXIT_FAILURE);
		}
		curssc.tc = dgettimeofday();
		curssc.t = ss.t;
		curssc.a = ss.a;
		curssc.b = ss.b;
		curssc.c = ss.c;
		en = pthread_mutex_unlock(&snaplock);
		if (en != 0) {
			fprintf(stderr, "pthread_mutex_unlock: %s\n", strerror(en));
			exit(EXIT_FAILURE);
		}
		curssc.sequence = curseq;
		curssc.iserror = 0;
		if ((curssc.t > curssc.tc) ||
		    modgreater(ssc[i].a, curssc.a) ||
		    modgreater(ssc[i].b, curssc.b) ||
		    modgreater(ssc[i].c, curssc.c) ||
		    modgreater(curssc.a, ssc[i].a + maxdelta) ||
		    modgreater(curssc.b, ssc[i].b + maxdelta) ||
		    modgreater(curssc.c, ssc[i].c + maxdelta)) {
			i++;
			curssc.iserror = 1;
		} else if (ssc[i].iserror)
			i++;
		ssc[i] = curssc;
		curseq++;
		if (i + 1 >= NSNAPS)
			break;
	}
	printf("consumer exited loop, collected %d items out of %d\n", i, curseq);
	if (ssc[0].iserror)
		printf("0/%ld: %.6f %.6f (%.3f) %ld %ld %ld\n",
		       ssc[0].sequence,
		       ssc[j].t, ssc[j].tc,
		       (ssc[j].tc - ssc[j].t) * 1000000,
		       ssc[j].a, ssc[j].b, ssc[j].c);
	for (j = 0; j <= i; j++)
		if (ssc[j].iserror)
			printf("%d/%ld: %.6f (%.3f) %ld %ld %ld\n",
			       j, ssc[j].sequence,
			       ssc[j].t,
			       (ssc[j].tc - ssc[j].t) * 1000000,
			       ssc[j].a - ssc[j - 1].a,
			       ssc[j].b - ssc[j - 1].b,
			       ssc[j].c - ssc[j - 1].c);
	consumer_done = 1;
	return NULL;
}

static volatile int watcher_ready = 0;

static void *watcher(void *ignored)
{
	long sum = 0;

	watcher_ready++;
	while (!goflag)
		sum += ss.a + ss.b + ss.c;
	return (void *)sum;
}

int main(int argc, char *argv[])
{
	int en;
	long i;
	pthread_t id;
	double starttime;

	(void)argc;
	(void)argv;

	en = pthread_create(&id, NULL, producer, NULL);
	if (en != 0) {
		fprintf(stderr, "pthread_create: %s\n", strerror(en));
		exit(EXIT_FAILURE);
	}
	en = pthread_create(&id, NULL, consumer, NULL);
	if (en != 0) {
		fprintf(stderr, "pthread_create: %s\n", strerror(en));
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < NWATCHERS; i++) {
		en = pthread_create(&id, NULL, watcher, (void *)i);
		if (en != 0) {
			fprintf(stderr, "pthread_create: %s\n", strerror(en));
			exit(EXIT_FAILURE);
		}
	}
	while (!producer_ready || !consumer_ready)
		sched_yield();
	goflag = 1;
	starttime = dgettimeofday();
	while ((dgettimeofday() - starttime < 3) && !consumer_done)
		poll(NULL, 0, 1);
	goflag = 0;
	while (!consumer_done || !producer_done)
		poll(NULL, 0, 1);
	return EXIT_SUCCESS;
}
