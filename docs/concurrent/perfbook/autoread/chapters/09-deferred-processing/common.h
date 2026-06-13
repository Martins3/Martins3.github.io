/* common.h: Common definitions for Deferred Processing demos
 *
 * Simplified from perfbook CodeSamples/api-pthreads/api-pthreads.h
 */

#ifndef COMMON_H
#define COMMON_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <unistd.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdatomic.h>
#include <limits.h>
#include <sched.h>

/* Linux-kernel-style macros */
#define DEFINE_SPINLOCK(name) pthread_mutex_t name = PTHREAD_MUTEX_INITIALIZER
#define spin_lock(lock) pthread_mutex_lock(lock)
#define spin_unlock(lock) pthread_mutex_unlock(lock)

#define smp_mb() __atomic_thread_fence(__ATOMIC_SEQ_CST)
#define smp_wmb() __atomic_thread_fence(__ATOMIC_RELEASE)
#define smp_rmb() __atomic_thread_fence(__ATOMIC_ACQUIRE)

#define READ_ONCE(x) \
	__atomic_load_n(&(x), __ATOMIC_RELAXED)
#define WRITE_ONCE(x, v) \
	__atomic_store_n(&(x), (v), __ATOMIC_RELAXED)

/* Atomic operations */
typedef struct { atomic_int counter; } atomic_t;

#define atomic_set(v, i) atomic_store_explicit(&(v)->counter, (i), __ATOMIC_RELAXED)
#define atomic_read(v)   atomic_load_explicit(&(v)->counter, __ATOMIC_RELAXED)
#define atomic_inc(v)    atomic_fetch_add_explicit(&(v)->counter, 1, __ATOMIC_RELAXED)
#define atomic_dec(v)    atomic_fetch_sub_explicit(&(v)->counter, 1, __ATOMIC_RELAXED)

static inline int atomic_dec_and_test(atomic_t *v)
{
	return atomic_fetch_sub_explicit(&(v)->counter, 1, __ATOMIC_ACQ_REL) == 1;
}

static inline int atomic_cmpxchg(atomic_t *v, int old, int new)
{
	atomic_compare_exchange_strong_explicit(
		&(v)->counter, &old, new,
		__ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
	return old;
}

/* Simple linked list (singly linked) */
struct list_head {
	struct list_head *next;
};

#define LIST_HEAD(name) struct list_head name = { &(name) }

static inline void list_add(struct list_head *new, struct list_head *head)
{
	new->next = head->next;
	head->next = new;
}

static inline void list_del(struct list_head *entry)
{
	struct list_head *p = entry;
	while (p->next != entry)
		p = p->next;
	p->next = entry->next;
}

/* Simple circular doubly-linked list for RCU demo */
struct cds_list_head {
	struct cds_list_head *next, *prev;
};

#define CDS_LIST_HEAD(name) struct cds_list_head name = { &(name), &(name) }

static inline void cds_list_add(struct cds_list_head *new, struct cds_list_head *head)
{
	new->next = head->next;
	new->prev = head;
	head->next->prev = new;
	head->next = new;
}

static inline void cds_list_del(struct cds_list_head *entry)
{
	entry->next->prev = entry->prev;
	entry->prev->next = entry->next;
}

#define cds_list_for_each_entry(pos, head, member) \
	for (pos = container_of((head)->next, typeof(*pos), member); \
	     &pos->member != (head); \
	     pos = container_of(pos->member.next, typeof(*pos), member))

#define container_of(ptr, type, member) ({ \
	const typeof(((type *)0)->member) *__mptr = (ptr); \
	(type *)((char *)__mptr - offsetof(type, member)); })

/* Barrier for compiler */
#define barrier() __asm__ __volatile__("" ::: "memory")

/* Thread helpers */
#define smp_thread_id() ((int)pthread_self())

static inline void set_thread_affinity(int cpu)
{
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cpu, &cpuset);
	pthread_setaffinity_np(pthread_self(), sizeof(cpuset), &cpuset);
}

#endif /* COMMON_H */
