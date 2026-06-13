/*
 * route_seqlock.c: Sequence-Locked Pre-BSD Routing Table (BUGGY!!!)
 *
 * Sequence locks detect concurrent updates and force readers to retry.
 * However, they do NOT provide existence guarantees: a reader can
 * dereference a freed pointer before read_seqretry() detects the race.
 */

#include "common.h"

typedef struct {
	unsigned long seq;
	pthread_mutex_t lock;
} seqlock_t;

#define DEFINE_SEQ_LOCK(name) seqlock_t name = { .seq = 0, .lock = PTHREAD_MUTEX_INITIALIZER }

static inline unsigned long read_seqbegin(seqlock_t *slp)
{
	unsigned long s = READ_ONCE(slp->seq);
	smp_mb();
	return s & ~0x1UL;
}

static inline int read_seqretry(seqlock_t *slp, unsigned long oldseq)
{
	smp_mb();
	unsigned long s = READ_ONCE(slp->seq);
	return s != oldseq;
}

static inline void write_seqlock(seqlock_t *slp)
{
	pthread_mutex_lock(&slp->lock);
	WRITE_ONCE(slp->seq, READ_ONCE(slp->seq) + 1);
	smp_mb();
}

static inline void write_sequnlock(seqlock_t *slp)
{
	smp_mb();
	WRITE_ONCE(slp->seq, READ_ONCE(slp->seq) + 1);
	pthread_mutex_unlock(&slp->lock);
}

struct route_entry {
	struct route_entry *re_next;
	unsigned long addr;
	unsigned long iface;
	int re_freed;
};

static struct route_entry route_list;
static DEFINE_SEQ_LOCK(sl);

unsigned long route_lookup(unsigned long addr)
{
	struct route_entry *rep;
	struct route_entry **repp;
	unsigned long ret;
	unsigned long s;

retry:
	s = read_seqbegin(&sl);
	repp = &route_list.re_next;
	do {
		rep = READ_ONCE(*repp);
		if (rep == NULL) {
			if (read_seqretry(&sl, s))
				goto retry;
			return ULONG_MAX;
		}
		repp = &rep->re_next;
	} while (rep->addr != addr);

	if (READ_ONCE(rep->re_freed))
		abort();
	ret = rep->iface;
	if (read_seqretry(&sl, s))
		goto retry;
	return ret;
}

int route_add(unsigned long addr, unsigned long interface)
{
	struct route_entry *rep = malloc(sizeof(*rep));
	if (!rep)
		return -ENOMEM;
	rep->addr = addr;
	rep->iface = interface;
	rep->re_freed = 0;
	write_seqlock(&sl);
	rep->re_next = route_list.re_next;
	route_list.re_next = rep;
	write_sequnlock(&sl);
	return 0;
}

int route_del(unsigned long addr)
{
	struct route_entry *rep;
	struct route_entry **repp;

	write_seqlock(&sl);
	repp = &route_list.re_next;
	for (;;) {
		rep = *repp;
		if (rep == NULL)
			break;
		if (rep->addr == addr) {
			*repp = rep->re_next;
			write_sequnlock(&sl);
			smp_mb();
			rep->re_freed = 1;
			free(rep);
			return 0;
		}
		repp = &rep->re_next;
	}
	write_sequnlock(&sl);
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
	}
	return NULL;
}

int main(void)
{
	pthread_t r1, r2, u1;

	printf("=== Sequence-Locked Routing Table (BUGGY) ===\n");
	route_add(42, 1);
	route_add(56, 3);
	route_add(17, 7);

	pthread_create(&r1, NULL, reader_thread, NULL);
	pthread_create(&r2, NULL, reader_thread, NULL);
	pthread_create(&u1, NULL, updater_thread, NULL);

	pthread_join(r1, NULL);
	pthread_join(r2, NULL);
	pthread_join(u1, NULL);

	printf("Stress test completed.\n");
	printf("Note: This can still crash with use-after-free because\n");
	printf("the reader may access a freed node BEFORE read_seqretry detects the race.\n");
	return 0;
}
