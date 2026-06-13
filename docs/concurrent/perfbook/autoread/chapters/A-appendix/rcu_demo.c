/*
 * rcu_demo.c: Toy RCU implementations based on perfbook appendix/toyrcu.
 *
 * This demo implements three progressively better RCU variants:
 * 1. Lock-Based RCU        - global spinlock (simplest, no concurrency)
 * 2. Per-Thread Lock RCU   - one lock per thread (readers can proceed in parallel)
 * 3. Quiescent-State RCU   - rcu_read_lock() is empty, uses explicit quiescent states
 *
 * Build: make rcu_demo.out
 * Run:   ./rcu_demo.out
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>

/* ---------- Shared infrastructure ---------- */

#define MAX_THREADS 64

static __thread int my_tid = -1;
static int nthreads = 0;

static void register_thread(void)
{
	static pthread_mutex_t reg_mutex = PTHREAD_MUTEX_INITIALIZER;
	static int next_tid = 0;
	pthread_mutex_lock(&reg_mutex);
	my_tid = next_tid++;
	if (my_tid >= MAX_THREADS) {
		fprintf(stderr, "Too many threads\n");
		exit(1);
	}
	nthreads = next_tid;
	pthread_mutex_unlock(&reg_mutex);
}

static inline int tid(void)
{
	if (my_tid < 0)
		register_thread();
	return my_tid;
}

static inline void barrier(void)
{
	__asm__ __volatile__("" ::: "memory");
}

/* ---------- Lock-Based RCU (simplest) ---------- */

static pthread_mutex_t rcu_gp_lock = PTHREAD_MUTEX_INITIALIZER;

static void rcu_lock_read_lock(void)
{
	pthread_mutex_lock(&rcu_gp_lock);
}

static void rcu_lock_read_unlock(void)
{
	pthread_mutex_unlock(&rcu_gp_lock);
}

static void rcu_lock_synchronize(void)
{
	pthread_mutex_lock(&rcu_gp_lock);
	pthread_mutex_unlock(&rcu_gp_lock);
}

/* ---------- Per-Thread Lock RCU ---------- */

static pthread_mutex_t rcu_perthread_locks[MAX_THREADS];
static int rcu_perthread_inited = 0;
static pthread_mutex_t rcu_perthread_init_mutex = PTHREAD_MUTEX_INITIALIZER;

static void rcu_perthread_init(void)
{
	pthread_mutex_lock(&rcu_perthread_init_mutex);
	if (!rcu_perthread_inited) {
		for (int i = 0; i < MAX_THREADS; i++)
			pthread_mutex_init(&rcu_perthread_locks[i], NULL);
		rcu_perthread_inited = 1;
	}
	pthread_mutex_unlock(&rcu_perthread_init_mutex);
}

static void rcu_perthread_read_lock(void)
{
	rcu_perthread_init();
	pthread_mutex_lock(&rcu_perthread_locks[tid()]);
}

static void rcu_perthread_read_unlock(void)
{
	pthread_mutex_unlock(&rcu_perthread_locks[tid()]);
}

static void rcu_perthread_synchronize(void)
{
	rcu_perthread_init();
	for (int i = 0; i < nthreads; i++) {
		pthread_mutex_lock(&rcu_perthread_locks[i]);
		pthread_mutex_unlock(&rcu_perthread_locks[i]);
	}
}

/* ---------- Quiescent-State RCU ---------- */

static long rcu_qs_gp_ctr = 0;
static __thread long rcu_qs_reader_gp = 0;

static void rcu_qs_quiescent_state(void)
{
	barrier();
	rcu_qs_reader_gp = __atomic_load_n(&rcu_qs_gp_ctr, __ATOMIC_RELAXED) + 1;
	barrier();
}

static void rcu_qs_thread_online(void)
{
	rcu_qs_quiescent_state();
}

static void rcu_qs_thread_offline(void)
{
	barrier();
	rcu_qs_reader_gp = __atomic_load_n(&rcu_qs_gp_ctr, __ATOMIC_RELAXED);
	barrier();
}

/* rcu_read_lock/unlock are empty for QS-based RCU */
static void rcu_qs_read_lock(void) { }
static void rcu_qs_read_unlock(void) { }

static void rcu_qs_synchronize(void)
{
	barrier();
	pthread_mutex_lock(&rcu_gp_lock);

	long snap = __atomic_load_n(&rcu_qs_gp_ctr, __ATOMIC_RELAXED);
	__atomic_store_n(&rcu_qs_gp_ctr, snap + 2, __ATOMIC_RELAXED);
	barrier();

	for (int t = 0; t < nthreads; t++) {
		while ((rcu_qs_reader_gp & 1) &&
		       ((rcu_qs_reader_gp - (snap + 2)) < 0)) {
			barrier();
		}
	}

	pthread_mutex_unlock(&rcu_gp_lock);
	barrier();
}

/* ---------- Test harness ---------- */

static volatile int data = 0;
static volatile int data_ready = 0;
static volatile int test_done = 0;

static void *reader_lock(void *arg)
{
	(void)arg;
	register_thread();
	while (!test_done) {
		rcu_lock_read_lock();
		if (data_ready)
			(void)data;
		rcu_lock_read_unlock();
	}
	return NULL;
}

static void *reader_perthread(void *arg)
{
	(void)arg;
	register_thread();
	while (!test_done) {
		rcu_perthread_read_lock();
		if (data_ready)
			(void)data;
		rcu_perthread_read_unlock();
	}
	return NULL;
}

static void *reader_qs(void *arg)
{
	(void)arg;
	register_thread();
	rcu_qs_thread_online();
	while (!test_done) {
		rcu_qs_read_lock();
		if (data_ready)
			(void)data;
		rcu_qs_read_unlock();
		rcu_qs_quiescent_state();
	}
	rcu_qs_thread_offline();
	return NULL;
}

static void test_rcu(const char *name,
		     void *(*reader_fn)(void *),
		     void (*sync_fn)(void),
		     int nreaders)
{
	pthread_t readers[MAX_THREADS];
	int i;

	data = 0;
	data_ready = 0;
	test_done = 0;
	my_tid = -1;
	nthreads = 0;

	printf("\n--- %s ---\n", name);
	for (i = 0; i < nreaders; i++)
		pthread_create(&readers[i], NULL, reader_fn, NULL);

	/* Allow readers to start */
	usleep(10000);

	/* Update side */
	for (int iter = 0; iter < 100; iter++) {
		data_ready = 0;
		data = iter + 1;
		data_ready = 1;
		sync_fn();
	}

	test_done = 1;
	for (i = 0; i < nreaders; i++)
		pthread_join(readers[i], NULL);

	printf("completed %d iterations with %d readers\n", 100, nreaders);
}

int main(int argc, char *argv[])
{
	int nreaders = 4;
	if (argc > 1)
		nreaders = atoi(argv[1]);
	if (nreaders < 1 || nreaders > MAX_THREADS)
		nreaders = 4;

	printf("Toy RCU demo (%d readers)\n", nreaders);

	test_rcu("Lock-Based RCU", reader_lock, rcu_lock_synchronize, nreaders);
	test_rcu("Per-Thread Lock RCU", reader_perthread, rcu_perthread_synchronize,
		 nreaders);
	test_rcu("Quiescent-State RCU", reader_qs, rcu_qs_synchronize, nreaders);

	return 0;
}
