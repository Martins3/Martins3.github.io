/*
 * lock.c: Demonstrate POSIX pthread_mutex_lock() and pthread_rwlock.
 * Shows exclusive locking and the effect of using same vs different locks.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <poll.h>

/* READ_ONCE / WRITE_ONCE equivalents for userspace */
#define READ_ONCE(x) (*(volatile typeof(x) *)&(x))
#define WRITE_ONCE(x, val) (*(volatile typeof(x) *)&(x) = (val))

pthread_mutex_t lock_a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_b = PTHREAD_MUTEX_INITIALIZER;

int x = 0;

void *lock_reader(void *arg)
{
	int en;
	int i;
	int newx = -1;
	int oldx = -1;
	pthread_mutex_t *pmlp = (pthread_mutex_t *)arg;

	en = pthread_mutex_lock(pmlp);
	if (en != 0) {
		fprintf(stderr, "lock_reader: pthread_mutex_lock: %s\n", strerror(en));
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < 100; i++) {
		newx = READ_ONCE(x);
		if (newx != oldx) {
			printf("lock_reader(): x = %d\n", newx);
		}
		oldx = newx;
		poll(NULL, 0, 1); /* sleep 1ms */
	}
	en = pthread_mutex_unlock(pmlp);
	if (en != 0) {
		fprintf(stderr, "lock_reader: pthread_mutex_unlock: %s\n", strerror(en));
		exit(EXIT_FAILURE);
	}
	return NULL;
}

void *lock_writer(void *arg)
{
	int en;
	int i;
	pthread_mutex_t *pmlp = (pthread_mutex_t *)arg;

	en = pthread_mutex_lock(pmlp);
	if (en != 0) {
		fprintf(stderr, "lock_writer: pthread_mutex_lock: %s\n", strerror(en));
		exit(EXIT_FAILURE);
	}
	for (i = 0; i < 3; i++) {
		WRITE_ONCE(x, READ_ONCE(x) + 1);
		poll(NULL, 0, 5); /* sleep 5ms */
	}
	en = pthread_mutex_unlock(pmlp);
	if (en != 0) {
		fprintf(stderr, "lock_writer: pthread_mutex_unlock: %s\n", strerror(en));
		exit(EXIT_FAILURE);
	}
	return NULL;
}

int main(void)
{
	pthread_t tid1, tid2;
	int en;
	void *vp;

	printf("=== Same lock: reader will only see final value ===\n");
	x = 0;
	en = pthread_create(&tid1, NULL, lock_reader, &lock_a);
	if (en != 0) { fprintf(stderr, "pthread_create: %s\n", strerror(en)); exit(EXIT_FAILURE); }
	en = pthread_create(&tid2, NULL, lock_writer, &lock_a);
	if (en != 0) { fprintf(stderr, "pthread_create: %s\n", strerror(en)); exit(EXIT_FAILURE); }
	pthread_join(tid1, &vp);
	pthread_join(tid2, &vp);

	printf("\n=== Different locks: reader may see intermediate values ===\n");
	x = 0;
	en = pthread_create(&tid1, NULL, lock_reader, &lock_a);
	if (en != 0) { fprintf(stderr, "pthread_create: %s\n", strerror(en)); exit(EXIT_FAILURE); }
	en = pthread_create(&tid2, NULL, lock_writer, &lock_b);
	if (en != 0) { fprintf(stderr, "pthread_create: %s\n", strerror(en)); exit(EXIT_FAILURE); }
	pthread_join(tid1, &vp);
	pthread_join(tid2, &vp);

	return 0;
}
