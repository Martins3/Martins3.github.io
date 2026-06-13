/*
 * assertion_demo.c: Demonstrate assertion mechanisms similar to Linux kernel.
 *
 * WARN_ON_ONCE: warn once per runtime (simplified version).
 * lockdep_assert_held: assert that a lock is held (simplified version).
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdatomic.h>
#include <pthread.h>

/* Simplified WARN_ON_ONCE using atomic exchange */
#define WARN_ON_ONCE(condition)                                         \
	do {                                                                \
		static atomic_int __warned = 0;                                 \
		if (condition) {                                                \
			if (atomic_exchange(&__warned, 1) == 0) {                   \
				fprintf(stderr, "WARN_ON_ONCE: %s at %s:%d\n",         \
					#condition, __FILE__, __LINE__);                \
			}                                                           \
		}                                                               \
	} while (0)

/* Simplified lockdep_assert_held using a per-lock owner field */
struct simple_lock {
	pthread_mutex_t mutex;
	pthread_t owner;
};

static void simple_lock_init(struct simple_lock *lock)
{
	pthread_mutex_init(&lock->mutex, NULL);
	lock->owner = 0;
}

static void simple_lock_acquire(struct simple_lock *lock)
{
	pthread_mutex_lock(&lock->mutex);
	lock->owner = pthread_self();
}

static void simple_lock_release(struct simple_lock *lock)
{
	lock->owner = 0;
	pthread_mutex_unlock(&lock->mutex);
}

#define lockdep_assert_held(lock)                                       \
	do {                                                                \
		if ((lock)->owner != pthread_self()) {                          \
			fprintf(stderr, "lockdep_assert_held failed at %s:%d "  \
				"(lock not held by current thread)\n",           \
				__FILE__, __LINE__);                            \
			abort();                                                    \
		}                                                               \
	} while (0)

static struct simple_lock g_lock;
static int g_data = 0;

/* Function that requires the caller to hold g_lock */
static void increment_data_locked(void)
{
	lockdep_assert_held(&g_lock);
	g_data++;
}

static void *worker_correct(void *arg)
{
	(void)arg;
	simple_lock_acquire(&g_lock);
	increment_data_locked();
	simple_lock_release(&g_lock);
	return NULL;
}

static void *worker_buggy(void *arg)
{
	(void)arg;
	/* Bug: calling increment_data_locked without holding the lock */
	increment_data_locked();
	return NULL;
}

int main(void)
{
	pthread_t t1, t2;

	simple_lock_init(&g_lock);

	printf("=== Test 1: correct usage ===\n");
	simple_lock_acquire(&g_lock);
	increment_data_locked();
	simple_lock_release(&g_lock);
	printf("data=%d (expected 1)\n", g_data);

	printf("\n=== Test 2: WARN_ON_ONCE ===\n");
	WARN_ON_ONCE(g_data != 100);
	WARN_ON_ONCE(g_data != 100); /* Should not print again */

	printf("\n=== Test 3: multithreaded correct usage ===\n");
	pthread_create(&t1, NULL, worker_correct, NULL);
	pthread_create(&t2, NULL, worker_correct, NULL);
	pthread_join(t1, NULL);
	pthread_join(t2, NULL);
	printf("data=%d (expected 3)\n", g_data);

	printf("\n=== Test 4: buggy usage (will abort) ===\n");
	/*
	 * Uncomment the following line to demonstrate lockdep_assert_held
	 * catching a bug:
	 *
	 * pthread_create(&t1, NULL, worker_buggy, NULL);
	 * pthread_join(t1, NULL);
	 */
	printf("(skipped to avoid abort)\n");

	pthread_mutex_destroy(&g_lock.mutex);
	return 0;
}
