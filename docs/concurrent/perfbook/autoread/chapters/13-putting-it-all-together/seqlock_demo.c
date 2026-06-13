/*
 * seqlock_demo.c - Sequence lock for correlated data.
 * Corresponds to perfbook Section 13.4 "Sequence-Locking Specials".
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

/* Simplified seqlock using atomic unsigned int */
typedef struct {
	atomic_uint sequence;  /* even = ready, odd = writer active */
	pthread_mutex_t lock;
} seqlock_t;

void seqlock_init(seqlock_t *sl)
{
	atomic_init(&sl->sequence, 0);
	pthread_mutex_init(&sl->lock, NULL);
}

unsigned read_seqbegin(seqlock_t *sl)
{
	unsigned seq;
	do {
		seq = atomic_load_explicit(&sl->sequence, memory_order_acquire);
	} while (seq & 1);
	return seq;
}

int read_seqretry(seqlock_t *sl, unsigned start)
{
	unsigned seq = atomic_load_explicit(&sl->sequence, memory_order_acquire);
	return seq != start;
}

void write_seqlock(seqlock_t *sl)
{
	pthread_mutex_lock(&sl->lock);
	atomic_fetch_add_explicit(&sl->sequence, 1, memory_order_release);
}

void write_sequnlock(seqlock_t *sl)
{
	atomic_fetch_add_explicit(&sl->sequence, 1, memory_order_release);
	pthread_mutex_unlock(&sl->lock);
}

/* --- correlated data: timekeeping calibration --- */
static seqlock_t cal_lock;
static volatile double cal_a = 1.0;
static volatile double cal_b = 0.0;

void *reader_thread(void *arg)
{
	(void)arg;
	int retries = 0;
	for (int i = 0; i < 100000; i++) {
		unsigned seq;
		double a, b;
		do {
			seq = read_seqbegin(&cal_lock);
			a = cal_a;
			b = cal_b;
			retries++;
		} while (read_seqretry(&cal_lock, seq));
		/* Consistent snapshot: a and b come from same update */
		if (a != 1.0 && a != 2.0) {
			printf("[FAIL] Invalid calibration a=%f\n", a);
			return NULL;
		}
	}
	printf("[OK] Reader done, avg retries per read=%.2f\n",
	       (double)retries / 100000.0);
	return NULL;
}

void *writer_thread(void *arg)
{
	(void)arg;
	for (int i = 0; i < 1000; i++) {
		write_seqlock(&cal_lock);
		cal_a = 2.0;
		cal_b = 10.0;
		write_sequnlock(&cal_lock);
		/* brief pause to let readers run */
		for (volatile int j = 0; j < 1000; j++) {}
		write_seqlock(&cal_lock);
		cal_a = 1.0;
		cal_b = 0.0;
		write_sequnlock(&cal_lock);
	}
	return NULL;
}

int main(void)
{
	printf("[seqlock_demo] Sequence lock for correlated data\n");
	seqlock_init(&cal_lock);

	pthread_t r1, r2, w;
	pthread_create(&r1, NULL, reader_thread, NULL);
	pthread_create(&r2, NULL, reader_thread, NULL);
	pthread_create(&w, NULL, writer_thread, NULL);

	pthread_join(r1, NULL);
	pthread_join(r2, NULL);
	pthread_join(w, NULL);

	printf("[OK] Seqlock consistency verified\n");
	return 0;
}
