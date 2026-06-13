/*
 * route_rcu.c: RCU Pre-BSD Routing Table (Toy Implementation)
 *
 * This uses a toy RCU implementation based on per-thread nesting counters.
 * In a real kernel, rcu_read_lock()/rcu_read_unlock() are much
 * more sophisticated, but the core idea is the same:
 * readers proceed locklessly, updaters wait for a grace period.
 */

#include "common.h"

#define RCU_MAX_THREADS 64

/* Toy RCU implementation for userspace */
static _Atomic int rcu_reader_nesting[RCU_MAX_THREADS];
static pthread_mutex_t rcu_mutex = PTHREAD_MUTEX_INITIALIZER;

static inline void rcu_read_lock(void)
{
	int tid = smp_thread_id() % RCU_MAX_THREADS;
	atomic_fetch_add_explicit(&rcu_reader_nesting[tid], 1, __ATOMIC_ACQUIRE);
}

static inline void rcu_read_unlock(void)
{
	int tid = smp_thread_id() % RCU_MAX_THREADS;
	atomic_fetch_sub_explicit(&rcu_reader_nesting[tid], 1, __ATOMIC_RELEASE);
}

static void synchronize_rcu(void)
{
	pthread_mutex_lock(&rcu_mutex);
	for (int t = 0; t < RCU_MAX_THREADS; t++) {
		while (atomic_load_explicit(&rcu_reader_nesting[t], __ATOMIC_ACQUIRE) != 0)
			sched_yield();
	}
	pthread_mutex_unlock(&rcu_mutex);
}

#define rcu_assign_pointer(p, v) \
	do { smp_wmb(); (p) = (v); } while (0)

#define rcu_dereference(p) \
	({ typeof(p) _________p1 = READ_ONCE(p); smp_rmb(); _________p1; })

/* Callback queue */
typedef void (*rcu_callback_t)(void *arg);

struct rcu_head {
	struct rcu_head *next;
	rcu_callback_t func;
	void *arg;
};

static struct rcu_head *rcu_cblist = NULL;

static void call_rcu(struct rcu_head *head, void (*func)(struct rcu_head *))
{
	head->func = (rcu_callback_t)func;
	head->arg = head;
	head->next = rcu_cblist;
	rcu_cblist = head;
}

static void rcu_invoke_callbacks(void)
{
	struct rcu_head *head = rcu_cblist;
	rcu_cblist = NULL;
	while (head) {
		struct rcu_head *next = head->next;
		head->func(head->arg);
		head = next;
	}
}

/* Route table */
struct route_entry {
	struct rcu_head rh;
	struct route_entry *re_next;
	unsigned long addr;
	unsigned long iface;
	int re_freed;
};

static struct route_entry *route_list = NULL;
static DEFINE_SPINLOCK(routelock);

static void re_free(struct route_entry *rep)
{
	WRITE_ONCE(rep->re_freed, 1);
	free(rep);
}

static void route_cb(struct rcu_head *rhp)
{
	struct route_entry *rep = (struct route_entry *)rhp;
	re_free(rep);
}

unsigned long route_lookup(unsigned long addr)
{
	struct route_entry *rep;
	unsigned long ret;

	rcu_read_lock();
	rep = rcu_dereference(route_list);
	while (rep) {
		if (rep->addr == addr) {
			ret = rep->iface;
			if (READ_ONCE(rep->re_freed))
				abort();
			rcu_read_unlock();
			return ret;
		}
		rep = rcu_dereference(rep->re_next);
	}
	rcu_read_unlock();
	return ULONG_MAX;
}

int route_add(unsigned long addr, unsigned long interface)
{
	struct route_entry *rep = malloc(sizeof(*rep));
	if (!rep)
		return -ENOMEM;
	rep->addr = addr;
	rep->iface = interface;
	rep->re_freed = 0;
	spin_lock(&routelock);
	rep->re_next = route_list;
	rcu_assign_pointer(route_list, rep);
	spin_unlock(&routelock);
	return 0;
}

int route_del(unsigned long addr)
{
	struct route_entry *rep;
	struct route_entry **repp;

	spin_lock(&routelock);
	repp = &route_list;
	for (;;) {
		rep = *repp;
		if (rep == NULL)
			break;
		if (rep->addr == addr) {
			*repp = rep->re_next;
			spin_unlock(&routelock);
			call_rcu(&rep->rh, route_cb);
			return 0;
		}
		repp = &rep->re_next;
	}
	spin_unlock(&routelock);
	return -ENOENT;
}

static void *reader_thread(void *arg)
{
	(void)arg;
	for (int i = 0; i < 100000; i++) {
		route_lookup(42);
		route_lookup(56);
	}
	return NULL;
}

static void *updater_thread(void *arg)
{
	(void)arg;
	for (int i = 0; i < 1000; i++) {
		route_del(56);
		route_add(56, 3);
		synchronize_rcu();
		rcu_invoke_callbacks();
	}
	return NULL;
}

int main(void)
{
	pthread_t r1, r2, u1;

	printf("=== RCU Routing Table (Toy) ===\n");
	for (int i = 0; i < RCU_MAX_THREADS; i++)
		atomic_store_explicit(&rcu_reader_nesting[i], 0, __ATOMIC_RELAXED);

	route_add(42, 1);
	route_add(56, 3);
	route_add(17, 7);

	pthread_create(&r1, NULL, reader_thread, NULL);
	pthread_create(&r2, NULL, reader_thread, NULL);
	pthread_create(&u1, NULL, updater_thread, NULL);

	pthread_join(r1, NULL);
	pthread_join(r2, NULL);
	pthread_join(u1, NULL);

	/* Final grace period and callback drain */
	synchronize_rcu();
	rcu_invoke_callbacks();

	printf("Stress test passed.\n");
	printf("Note: Readers use lightweight rcu_read_lock/rcu_read_unlock.\n");
	return 0;
}
