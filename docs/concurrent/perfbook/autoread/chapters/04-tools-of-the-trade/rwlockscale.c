/*
 * rwlockscale.c: Measure reader-writer lock scalability.
 * Based on perfbook CodeSamples/toolsoftrade/rwlockscale.c
 *
 * Reader-writer locks allow multiple concurrent readers, but
 * all threads must update the lock state, causing cache-line
 * bouncing and poor scalability on many CPUs.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#define READ_ONCE(x) (*(volatile typeof(x) *)&(x))

static inline void wait_microseconds(long us)
{
	usleep(us);
}

pthread_rwlock_t rwl = PTHREAD_RWLOCK_INITIALIZER;
unsigned long holdtime = 0;
unsigned long thinktime = 0;
long long *readcounts;
int nreadersrunning = 0;

#define GOFLAG_INIT 0
#define GOFLAG_RUN  1
#define GOFLAG_STOP 2
char goflag = GOFLAG_INIT;

void *reader(void *arg)
{
	int en;
	int i;
	long long loopcnt = 0;
	long me = (long)arg;

	__sync_fetch_and_add(&nreadersrunning, 1);
	while (READ_ONCE(goflag) == GOFLAG_INIT) {
		continue;
	}
	while (READ_ONCE(goflag) == GOFLAG_RUN) {
		en = pthread_rwlock_rdlock(&rwl);
		if (en != 0) {
			fprintf(stderr, "pthread_rwlock_rdlock: %s\n", strerror(en));
			exit(EXIT_FAILURE);
		}
		for (i = 1; i < holdtime; i++) {
			wait_microseconds(1);
		}
		en = pthread_rwlock_unlock(&rwl);
		if (en != 0) {
			fprintf(stderr, "pthread_rwlock_unlock: %s\n", strerror(en));
			exit(EXIT_FAILURE);
		}
		for (i = 1; i < thinktime; i++) {
			wait_microseconds(1);
		}
		loopcnt++;
	}
	readcounts[me] = loopcnt;
	return NULL;
}

int main(int argc, char *argv[])
{
	int en;
	long i;
	int nthreads;
	long long sum = 0LL;
	pthread_t *tid;
	void *vp;

	if (argc != 4) {
		fprintf(stderr, "Usage: %s nthreads holdloops thinkloops\n", argv[0]);
		exit(EXIT_FAILURE);
	}
	nthreads = strtoul(argv[1], NULL, 0);
	holdtime = strtoul(argv[2], NULL, 0);
	thinktime = strtoul(argv[3], NULL, 0);
	readcounts = calloc(nthreads, sizeof(readcounts[0]));
	tid = calloc(nthreads, sizeof(tid[0]));
	if (readcounts == NULL || tid == NULL) {
		fprintf(stderr, "Out of memory\n");
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < nthreads; i++) {
		en = pthread_create(&tid[i], NULL, reader, (void *)i);
		if (en != 0) {
			fprintf(stderr, "pthread_create: %s\n", strerror(en));
			exit(EXIT_FAILURE);
		}
	}
	while (READ_ONCE(nreadersrunning) < nthreads) {
		continue;
	}
	goflag = GOFLAG_RUN;
	sleep(1);
	goflag = GOFLAG_STOP;

	for (i = 0; i < nthreads; i++) {
		if ((en = pthread_join(tid[i], &vp)) != 0) {
			fprintf(stderr, "pthread_join: %s\n", strerror(en));
			exit(EXIT_FAILURE);
		}
	}
	for (i = 0; i < nthreads; i++) {
		sum += readcounts[i];
	}

	printf("n: %d  hold: %lu us  think: %lu us  total acquisitions: %lld\n",
	       nthreads, holdtime, thinktime, sum);

	free(readcounts);
	free(tid);
	return 0;
}
