/*
 * pcreate.c: Demonstrate POSIX pthread_create() and pthread_join().
 * Threads created via pthread_create() DO share memory.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

int x = 0;

void *mythread(void *arg)
{
	x = 1;
	printf("Child thread set x = 1\n");
	return NULL;
}

int main(void)
{
	pthread_t tid;
	int en;
	void *vp;

	en = pthread_create(&tid, NULL, mythread, NULL);
	if (en != 0) {
		fprintf(stderr, "pthread_create: %s\n", strerror(en));
		exit(EXIT_FAILURE);
	}

	en = pthread_join(tid, &vp);
	if (en != 0) {
		fprintf(stderr, "pthread_join: %s\n", strerror(en));
		exit(EXIT_FAILURE);
	}

	printf("Parent thread sees x = %d (expected 1, threads share memory)\n", x);

	return 0;
}
