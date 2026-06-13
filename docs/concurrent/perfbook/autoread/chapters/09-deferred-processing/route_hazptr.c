/*
 * route_hazptr.c: Hazard-Pointer Pre-BSD Routing Table
 *
 * Hazard pointers store per-thread pointers to objects being traversed.
 * An object can only be freed after scanning confirms no hazard pointer
 * references it.  Readers may need to retry on concurrent deletion.
 */

#include "common.h"

/* Hazard pointer parameters */
#define K 2
#define MAX_THREADS 64
#define H (K * MAX_THREADS)
#define R (100 + 2 * H)

typedef struct hazptr_head {
	struct hazptr_head *next;
} hazptr_head_t;

typedef struct {
	void *__attribute__((__aligned__(64))) p;
} hazard_pointer;

static hazard_pointer HP[H];
static hazptr_head_t *rlist[MAX_THREADS];
static int rcount[MAX_THREADS];
static pthread_mutex_t hazptr_mutex = PTHREAD_MUTEX_INITIALIZER;

#define HAZPTR_POISON ((void *)0x8)

static inline void *hp_try_record(void **p, hazard_pointer *hp)
{
	void *tmp = READ_ONCE(*p);
	if (!tmp || tmp == HAZPTR_POISON)
		return tmp;
	WRITE_ONCE(hp->p, tmp);
	smp_mb();
	if (tmp == READ_ONCE(*p))
		return tmp;
	return HAZPTR_POISON;
}

static inline void *hp_record(void **p, hazard_pointer *hp)
{
	void *tmp;
	do {
		tmp = hp_try_record(p, hp);
	} while (tmp == HAZPTR_POISON);
	return tmp;
}

static inline void hp_clear(hazard_pointer *hp)
{
	smp_mb();
	WRITE_ONCE(hp->p, NULL);
}

static int cmp_ptr(const void *a, const void *b)
{
	void * const *pa = a;
	void * const *pb = b;
	return (*pa > *pb) - (*pa < *pb);
}

static void hazptr_scan(void)
{
	void *plist[H];
	int psize = 0;
	int tid = smp_thread_id() % MAX_THREADS;

	smp_mb();
	for (int i = 0; i < H; i++) {
		void *p = READ_ONCE(HP[i].p);
		if (p)
			plist[psize++] = p;
	}
	smp_mb();
	qsort(plist, psize, sizeof(void *), cmp_ptr);

	hazptr_head_t *tmplist = rlist[tid];
	rlist[tid] = NULL;
	rcount[tid] = 0;

	hazptr_head_t *cur = tmplist;
	hazptr_head_t *next;
	while (cur) {
		next = cur->next;
		void *p = cur;
		int found = bsearch(&p, plist, psize, sizeof(void *), cmp_ptr) != NULL;
		if (found) {
			cur->next = rlist[tid];
			rlist[tid] = cur;
			rcount[tid]++;
		} else {
			free(p);
		}
		cur = next;
	}
}

static void hazptr_free_later(hazptr_head_t *hh)
{
	int tid = smp_thread_id() % MAX_THREADS;
	pthread_mutex_lock(&hazptr_mutex);
	hh->next = rlist[tid];
	rlist[tid] = hh;
	if (++rcount[tid] >= R)
		hazptr_scan();
	pthread_mutex_unlock(&hazptr_mutex);
}

struct route_entry {
	struct hazptr_head hh;
	struct route_entry *re_next;
	unsigned long addr;
	unsigned long iface;
	int re_freed;
};

static struct route_entry route_list;
static DEFINE_SPINLOCK(routelock);
static __thread hazard_pointer *my_hazptr;

unsigned long route_lookup(unsigned long addr)
{
	int offset = 0;
	struct route_entry *rep;
	struct route_entry **repp;

retry:
	repp = &route_list.re_next;
	do {
		rep = hp_try_record((void **)repp, &my_hazptr[offset]);
		if (!rep)
			return ULONG_MAX;
		if (rep == HAZPTR_POISON)
			goto retry;
		repp = &rep->re_next;
	} while (rep->addr != addr);

	if (READ_ONCE(rep->re_freed))
		abort();
	unsigned long ret = rep->iface;
	hp_clear(&my_hazptr[offset]);
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
	spin_lock(&routelock);
	rep->re_next = route_list.re_next;
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
			rep->re_next = (struct route_entry *)HAZPTR_POISON;
			spin_unlock(&routelock);
			hazptr_free_later(&rep->hh);
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
	my_hazptr = &HP[(smp_thread_id() % MAX_THREADS) * K];
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

	printf("=== Hazard-Pointer Routing Table ===\n");
	memset(HP, 0, sizeof(HP));
	memset(rlist, 0, sizeof(rlist));
	memset(rcount, 0, sizeof(rcount));

	route_add(42, 1);
	route_add(56, 3);
	route_add(17, 7);

	pthread_create(&r1, NULL, reader_thread, NULL);
	pthread_create(&r2, NULL, reader_thread, NULL);
	pthread_create(&u1, NULL, updater_thread, NULL);

	pthread_join(r1, NULL);
	pthread_join(r2, NULL);
	pthread_join(u1, NULL);

	printf("Stress test passed.\n");
	printf("Note: Readers write to per-thread hazard pointers, no writes to shared object.\n");
	return 0;
}
