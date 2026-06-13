/*
 * nbs_queue.c: Half-NBS queue with lock-free enqueue and blocking dequeue.
 *
 * Based on perfbook Chapter 14, Section 14.2.1.3 "Half-NBS Queue"
 * and the wfcq (wait-free concurrent queue) algorithm.
 *
 * Enqueue uses atomic xchg on tail; dequeue needs mutual exclusion.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

#define barrier() __asm__ __volatile__("" : : : "memory")

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))
#define READ_ONCE(x) ({ typeof(x) ___x = ACCESS_ONCE(x); ___x; })
#define WRITE_ONCE(x, val) do { ACCESS_ONCE(x) = (val); } while (0)

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

/* Queue element */
struct el {
	struct el *next;
	int val;
};

struct queue {
	struct el *head;
	struct el **tail;
};

static struct queue q;
static pthread_mutex_t q_lock = PTHREAD_MUTEX_INITIALIZER;

void init_q(struct queue *qp)
{
	struct el *dummy = calloc(1, sizeof(*dummy));
	if (!dummy) {
		perror("calloc");
		exit(1);
	}
	qp->head = dummy;
	qp->tail = &dummy->next;
	qp->head->next = NULL;
}

/* Lock-free enqueue using xchg */
int q_push(struct el *p, struct queue *qp)
{
	struct el **oldtail;

	p->next = NULL;
	oldtail = xchg(&qp->tail, (struct el **)p);
	*oldtail = p;
	return 1;
}

/* Blocking dequeue: requires external locking for single-element removal */
struct el *q_pop(struct queue *qp)
{
	struct el *p;
	struct el *pnext;

	pthread_mutex_lock(&q_lock);
	p = qp->head;
	pnext = p->next;
	if (pnext == NULL) {
		pthread_mutex_unlock(&q_lock);
		return NULL;
	}
	qp->head = pnext;
	pthread_mutex_unlock(&q_lock);

	(void)p; /* original dummy check omitted for simplicity */
	return pnext;
}

/* Non-blocking dequeue-all: atomically remove entire queue contents */
struct el *q_pop_all(struct queue *qp)
{
	struct el *old_head;
	struct el *old_tail;
	struct el dummy;

	/* Atomically exchange head with NULL */
	old_head = xchg(&qp->head, NULL);
	if (!old_head)
		return NULL;

	/* Atomically exchange tail to point to a dummy, capturing last node */
	dummy.next = NULL;
	old_tail = xchg(&qp->tail, (struct el **)&dummy.next);
	*(struct el **)old_tail = NULL;

	/* Skip the dummy head that was originally there */
	if (old_head->next == NULL) {
		/* Queue was empty except dummy */
		return NULL;
	}
	struct el *real_head = old_head->next;
	free(old_head);
	return real_head;
}

#define NR_THREADS 4
#define NR_PUSHES 100000
static pthread_t threads[NR_THREADS];

static void *enqueue_worker(void *arg)
{
	long tid = (long)arg;
	int i;

	for (i = 0; i < NR_PUSHES; i++) {
		struct el *p = malloc(sizeof(*p));
		if (!p) {
			perror("malloc");
			exit(1);
		}
		p->val = (int)(tid * NR_PUSHES + i);
		q_push(p, &q);
	}
	return NULL;
}

int main(void)
{
	long t;
	int total_expected = NR_THREADS * NR_PUSHES;
	int count = 0;

	printf("Half-NBS Queue Test: %d threads, %d enqueues each\n",
	       NR_THREADS, NR_PUSHES);

	init_q(&q);

	for (t = 0; t < NR_THREADS; t++)
		pthread_create(&threads[t], NULL, enqueue_worker, (void *)t);

	for (t = 0; t < NR_THREADS; t++)
		pthread_join(threads[t], NULL);

	struct el *all = q_pop_all(&q);
	while (all) {
		struct el *next = all->next;
		count++;
		free(all);
		all = next;
	}

	printf("Total nodes enqueued: %d\n", total_expected);
	printf("Total nodes dequeued: %d\n", count);
	printf("Result: %s\n", count == total_expected ? "PASS" : "FAIL");
	return (count == total_expected) ? 0 : 1;
}
