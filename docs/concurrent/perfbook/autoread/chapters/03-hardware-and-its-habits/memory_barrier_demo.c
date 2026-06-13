/*
 * memory_barrier_demo.c
 * Demonstrates the need for memory barriers in lock-based critical sections.
 *
 * perfbook Ch.3 "Memory Barriers" explains that without barriers, CPUs and
 * compilers may reorder memory operations, breaking synchronization.
 *
 * This program shows a simple spinlock-like pattern and explains why
 * barriers are needed. We use Linux kernel-style memory ordering to
 * show acquire/release semantics.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <stdatomic.h>

#define ITERATIONS 10000000

/*
 * Simple ticket lock using atomic operations with memory ordering.
 * This mirrors the spin_lock/spin_unlock example in perfbook.
 */
struct ticket_lock {
	atomic_uint ticket;
	atomic_uint serving;
};

static struct ticket_lock lock = {
	.ticket = ATOMIC_VAR_INIT(0),
	.serving = ATOMIC_VAR_INIT(0),
};

static uint64_t shared_data = 0;
static uint64_t total_sum = 0;

static void ticket_lock_acquire(struct ticket_lock *lk)
{
	unsigned int my_ticket = atomic_fetch_add_explicit(&lk->ticket, 1,
								memory_order_relaxed);
	while (atomic_load_explicit(&lk->serving, memory_order_acquire) != my_ticket) {
		/* Spin with acquire semantics */
	}
	/* Acquire barrier ensures all subsequent loads/stores happen after lock */
}

static void ticket_lock_release(struct ticket_lock *lk)
{
	unsigned int next = atomic_load_explicit(&lk->serving, memory_order_relaxed) + 1;
	/* Release barrier ensures all previous loads/stores happen before unlock */
	atomic_store_explicit(&lk->serving, next, memory_order_release);
}

static void *worker(void *arg)
{
	(void)arg;
	for (int i = 0; i < ITERATIONS; i++) {
		ticket_lock_acquire(&lock);
		shared_data++;
		total_sum += shared_data;
		ticket_lock_release(&lock);
	}
	return NULL;
}

static void run_with_lock(int nthreads)
{
	pthread_t *threads = malloc(nthreads * sizeof(pthread_t));
	shared_data = 0;
	total_sum = 0;
	atomic_store(&lock.ticket, 0);
	atomic_store(&lock.serving, 0);

	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	for (int i = 0; i < nthreads; i++) {
		pthread_create(&threads[i], NULL, worker, NULL);
	}
	for (int i = 0; i < nthreads; i++) {
		pthread_join(threads[i], NULL);
	}

	clock_gettime(CLOCK_MONOTONIC, &end);
	double elapsed = (end.tv_sec - start.tv_sec) * 1e9 +
			 (end.tv_nsec - start.tv_nsec);

	printf("Threads: %d, final value: %lu, time: %.3f ms\n",
	       nthreads, shared_data, elapsed / 1e6);

	free(threads);
}

/*
 * Demonstrate what happens with a "broken" lock that lacks barriers.
 * This is UNSAFE and may show incorrect results due to reordering.
 */
struct broken_lock {
	atomic_int flag;
};

static struct broken_lock b_lock = { .flag = ATOMIC_VAR_INIT(0) };
static uint64_t broken_data = 0;

static void broken_lock_acquire(struct broken_lock *lk)
{
	/* No acquire barrier - compiler/CPU may reorder! */
	while (atomic_exchange_explicit(&lk->flag, 1, memory_order_relaxed) == 1) {
		/* spin */
	}
}

static void broken_lock_release(struct broken_lock *lk)
{
	/* No release barrier */
	atomic_store_explicit(&lk->flag, 0, memory_order_relaxed);
}

static void *broken_worker(void *arg)
{
	(void)arg;
	for (int i = 0; i < ITERATIONS; i++) {
		broken_lock_acquire(&b_lock);
		broken_data++;
		broken_lock_release(&b_lock);
	}
	return NULL;
}

static void run_broken_lock(int nthreads)
{
	pthread_t *threads = malloc(nthreads * sizeof(pthread_t));
	broken_data = 0;
	atomic_store(&b_lock.flag, 0);

	for (int i = 0; i < nthreads; i++) {
		pthread_create(&threads[i], NULL, broken_worker, NULL);
	}
	for (int i = 0; i < nthreads; i++) {
		pthread_join(threads[i], NULL);
	}

	printf("Broken lock (no barriers): threads=%d, final value=%lu (expected=%d)\n",
	       nthreads, broken_data, nthreads * ITERATIONS);

	free(threads);
}

int main(void)
{
	printf("=== Memory Barrier Demo ===\n\n");

	printf("Proper ticket lock with acquire/release barriers:\n");
	run_with_lock(1);
	run_with_lock(2);
	run_with_lock(4);

	printf("\nNote: acquire in lock ensures loads/stores in critical section\n");
	printf("cannot be reordered before the lock is acquired.\n");
	printf("Release in unlock ensures they cannot be reordered after unlock.\n");

	printf("\n--- Broken lock (no barriers) ---\n");
	printf("Warning: may not always show wrong results due to timing,\n");
	printf("but is formally incorrect and unsafe on weakly-ordered CPUs.\n");
	run_broken_lock(2);
	run_broken_lock(4);

	return 0;
}
