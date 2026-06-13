/*
 * nbs_stack.c: Lock-free LIFO stack using compare-and-swap (CAS).
 *
 * Based on perfbook Chapter 14 (Advanced Synchronization),
 * Section 14.2.1.4 "NBS Stack".
 *
 * This demonstrates a lock-free push and bounded wait-free pop_all.
 * The ABA problem is discussed in the accompanying readme.md.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define NR_THREADS 4
#define NR_PUSHES 100000

/*
 * Basic compiler barriers and atomic primitives mimicking kernel APIs.
 */
#define barrier() __asm__ __volatile__("" : : : "memory")

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))
#define READ_ONCE(x) ({ typeof(x) ___x = ACCESS_ONCE(x); ___x; })
#define WRITE_ONCE(x, val) do { ACCESS_ONCE(x) = (val); } while (0)

static inline int cmpxchg(void *ptr, void *expected, void *newval)
{
	void *prev;
	__asm__ volatile(
		"lock; cmpxchgq %2, %1"
		: "=a" (prev), "+m" (*(void **)ptr)
		: "r" (newval), "0" (expected)
		: "memory");
	return prev == expected;
}

static inline void *xchg(void *ptr, void *newval)
{
	void *prev;
	__asm__ volatile(
		"lock; xchgq %0, %1"
		: "=r" (prev), "+m" (*(void **)ptr)
		: "0" (newval)
		: "memory");
	return prev;
}

/* Node structure for the LIFO stack */
struct node_t {
	int val;
	struct node_t *next;
};

static struct node_t *top;
static pthread_t threads[NR_THREADS];

void list_push(int v)
{
	struct node_t *newnode = malloc(sizeof(*newnode));
	struct node_t *oldtop;

	if (!newnode) {
		perror("malloc");
		exit(1);
	}
	newnode->val = v;
	oldtop = READ_ONCE(top);
	do {
		newnode->next = oldtop;
	} while (!cmpxchg(&top, oldtop, newnode) &&
		 (oldtop = READ_ONCE(top), 1));
}

/* Atomic pop_all: removes entire stack in one xchg */
struct node_t *list_pop_all(void)
{
	return xchg(&top, NULL);
}

static void verify_and_free(struct node_t *p)
{
	static _Atomic int seen_count;
	while (p) {
		struct node_t *next = p->next;
		seen_count++;
		free(p);
		p = next;
	}
}

static void *push_worker(void *arg)
{
	long tid = (long)arg;
	int i;

	for (i = 0; i < NR_PUSHES; i++)
		list_push((int)(tid * NR_PUSHES + i));
	return NULL;
}

int main(void)
{
	long t;
	int total_expected = NR_THREADS * NR_PUSHES;

	printf("NBS Stack Test: %d threads, %d pushes each\n",
	       NR_THREADS, NR_PUSHES);

	for (t = 0; t < NR_THREADS; t++)
		pthread_create(&threads[t], NULL, push_worker, (void *)t);

	for (t = 0; t < NR_THREADS; t++)
		pthread_join(threads[t], NULL);

	struct node_t *all = list_pop_all();
	int count = 0;
	struct node_t *p = all;
	while (p) {
		count++;
		p = p->next;
	}
	verify_and_free(all);

	printf("Total nodes pushed: %d\n", total_expected);
	printf("Total nodes popped: %d\n", count);
	printf("Result: %s\n", count == total_expected ? "PASS" : "FAIL");
	return (count == total_expected) ? 0 : 1;
}
