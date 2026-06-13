/*
 * route_refcnt.c: Reference-Counted Pre-BSD Routing Table (BUGGY!!!)
 *
 * This demonstrates the classic reference-counting trap:
 * The reference counter lives IN the object, so there is a window
 * between reading the pointer and acquiring the reference where
 * the object can be freed by a concurrent updater.
 */

#include "common.h"

struct route_entry {
	atomic_t re_refcnt;
	struct route_entry *re_next;
	unsigned long addr;
	unsigned long iface;
	int re_freed;
};

static struct route_entry route_list;
static DEFINE_SPINLOCK(routelock);

static void re_free(struct route_entry *rep)
{
	WRITE_ONCE(rep->re_freed, 1);
	free(rep);
}

unsigned long route_lookup(unsigned long addr)
{
	int old, new;
	struct route_entry *rep = NULL;
	struct route_entry **repp;
	unsigned long ret;

retry:
	repp = &route_list.re_next;
	do {
		if (rep && atomic_dec_and_test(&rep->re_refcnt))
			re_free(rep);
		rep = READ_ONCE(*repp);
		if (rep == NULL)
			return ULONG_MAX;

		/* Acquire a reference if count is non-zero */
		do {
			if (READ_ONCE(rep->re_freed)) {
				fprintf(stderr, "USE AFTER FREE detected!\n");
				abort();
			}
			old = atomic_read(&rep->re_refcnt);
			if (old <= 0)
				goto retry;
			new = old + 1;
		} while (atomic_cmpxchg(&rep->re_refcnt, old, new) != old);

		repp = &rep->re_next;
	} while (rep->addr != addr);

	ret = rep->iface;
	if (atomic_dec_and_test(&rep->re_refcnt))
		re_free(rep);
	return ret;
}

int route_add(unsigned long addr, unsigned long interface)
{
	struct route_entry *rep = malloc(sizeof(*rep));
	if (!rep)
		return -ENOMEM;
	atomic_set(&rep->re_refcnt, 1);
	rep->addr = addr;
	rep->iface = interface;
	spin_lock(&routelock);
	rep->re_next = route_list.re_next;
	rep->re_freed = 0;
	route_list.re_next = rep;
	spin_unlock(&routelock);
	return 0;
}

int route_del(unsigned long addr)
{
	struct route_entry *rep;
	struct route_entry **repp;

	spin_lock(&routelock);
	repp = &route_list.re_next;
	for (;;) {
		rep = *repp;
		if (rep == NULL)
			break;
		if (rep->addr == addr) {
			*repp = rep->re_next;
			spin_unlock(&routelock);
			if (atomic_dec_and_test(&rep->re_refcnt))
				re_free(rep);
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
		route_lookup(17);
	}
	return NULL;
}

static void *updater_thread(void *arg)
{
	(void)arg;
	for (int i = 0; i < 1000; i++) {
		route_del(56);
		route_add(56, 3);
	}
	return NULL;
}

int main(void)
{
	pthread_t r1, r2, u1;

	printf("=== Reference-Counted Routing Table (BUGGY) ===\n");
	route_add(42, 1);
	route_add(56, 3);
	route_add(17, 7);

	pthread_create(&r1, NULL, reader_thread, NULL);
	pthread_create(&r2, NULL, reader_thread, NULL);
	pthread_create(&u1, NULL, updater_thread, NULL);

	pthread_join(r1, NULL);
	pthread_join(r2, NULL);
	pthread_join(u1, NULL);

	printf("Survived stress test (but may have raced silently).\n");
	printf("Note: The bug is that between READ_ONCE(*repp) and atomic_cmpxchg,\n");
	printf("the object can be freed by a concurrent route_del().\n");
	return 0;
}
