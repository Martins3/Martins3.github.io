/*
 * urcu_simple.h - Simplified userspace RCU for demonstration.
 * This is NOT production-quality RCU. It uses a global mutex for
 * update-side serialization and a reader counter to emulate grace
 * periods.  It suffices to demonstrate the algorithms in Chapter 13.
 */
#ifndef URCU_SIMPLE_H
#define URCU_SIMPLE_H

#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <unistd.h>

struct rcu_head {
	void (*func)(struct rcu_head *head);
};

static atomic_int rcu_reader_count = 0;

static inline void rcu_read_lock(void)
{
	atomic_fetch_add_explicit(&rcu_reader_count, 1, memory_order_acquire);
}

static inline void rcu_read_unlock(void)
{
	atomic_fetch_sub_explicit(&rcu_reader_count, 1, memory_order_release);
}

static inline void synchronize_rcu(void)
{
	/* Wait until all previously-started readers have finished. */
	while (atomic_load_explicit(&rcu_reader_count, memory_order_relaxed) != 0)
		; /* spin */
}

/* Memory-barrier helpers */
#define smp_mb()        __atomic_thread_fence(memory_order_seq_cst)
#define smp_wmb()       __atomic_thread_fence(memory_order_release)
#define smp_rmb()       __atomic_thread_fence(memory_order_acquire)

#define rcu_assign_pointer(p, v) \
	do { smp_wmb(); (p) = (v); } while (0)

#define rcu_dereference(p) \
	({ __typeof__(p) ________p1 = (p); smp_rmb(); ________p1; })

#define WRITE_ONCE(x, val) \
	atomic_store_explicit((_Atomic __typeof__(x) *)&(x), (val), memory_order_relaxed)

#define READ_ONCE(x) \
	atomic_load_explicit((_Atomic __typeof__(x) *)&(x), memory_order_relaxed)

#endif /* URCU_SIMPLE_H */
