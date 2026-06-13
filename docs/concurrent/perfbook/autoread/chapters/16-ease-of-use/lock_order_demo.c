/*
 * Demonstration of lock ordering strategies for circular doubly-linked list.
 *
 * This program illustrates the "Shaving the Mandelbrot Set" concept from
 * Chapter 16 of perfbook:  a specialized, fragile algorithm vs. a simple,
 * robust one.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>

/*
 * Node in a circular doubly-linked list.
 * Each node has its own mutex for fine-grained locking.
 */
struct list_node {
	struct list_node *prev;
	struct list_node *next;
	pthread_mutex_t lock;
	int id;
};

/* Global list header */
static struct list_node list_head;
static int node_count = 0;
static pthread_mutex_t node_count_lock = PTHREAD_MUTEX_INITIALIZER;

/*
 * Initialize the circular list with a header node.
 */
static void list_init(struct list_node *head)
{
	head->prev = head;
	head->next = head;
	head->id = -1;
	pthread_mutex_init(&head->lock, NULL);
}

/*
 * Helper: lock two nodes in address order.
 * This is the ROBUST approach (Rusty Scale level 7/8).
 * It works for any list size and any number of threads.
 */
static void lock_two_nodes_address_order(struct list_node *a,
					  struct list_node *b)
{
	if (a == b) {
		pthread_mutex_lock(&a->lock);
	} else if ((uintptr_t)a < (uintptr_t)b) {
		pthread_mutex_lock(&a->lock);
		pthread_mutex_lock(&b->lock);
	} else {
		pthread_mutex_lock(&b->lock);
		pthread_mutex_lock(&a->lock);
	}
}

static void unlock_two_nodes(struct list_node *a, struct list_node *b)
{
	pthread_mutex_unlock(&a->lock);
	if (a != b)
		pthread_mutex_unlock(&b->lock);
}

/*
 * ROBUST insertion: acquire predecessor and successor locks in address order.
 * Simple, easy to understand, works for any list size.
 */
static void list_insert_robust(struct list_node *new,
			       struct list_node *pred,
			       struct list_node *succ)
{
	lock_two_nodes_address_order(pred, succ);

	new->prev = pred;
	new->next = succ;
	pred->next = new;
	succ->prev = new;

	unlock_two_nodes(pred, succ);
}

/*
 * FRAGILE insertion: acquire locks in list order (pred first, then succ).
 * This is the "to-be-shaven" algorithm from the chapter.
 *
 * The chapter proves that deadlock cannot occur IF:
 *   - the list is circular
 *   - there are exactly N threads
 *   - there are exactly N+1 elements (header + N nodes)
 *   - each thread locks a consecutive pair
 *
 * But this is extremely fragile: adding one extra thread or missing a node
 * insertion breaks the invariant and causes deadlock.
 */
static void list_insert_fragile(struct list_node *new,
				struct list_node *pred,
				struct list_node *succ)
{
	/* Always acquire predecessor first (list order) */
	pthread_mutex_lock(&pred->lock);
	if (pred != succ)
		pthread_mutex_lock(&succ->lock);

	new->prev = pred;
	new->next = succ;
	pred->next = new;
	succ->prev = new;

	if (pred != succ)
		pthread_mutex_unlock(&succ->lock);
	pthread_mutex_unlock(&pred->lock);
}

/*
 * Thread argument: each thread inserts nodes into the list.
 */
struct thread_arg {
	int thread_id;
	int insert_count;
	int use_fragile;
};

static void *inserter_thread(void *arg)
{
	struct thread_arg *ta = arg;

	for (int i = 0; i < ta->insert_count; i++) {
		struct list_node *new = malloc(sizeof(*new));
		if (!new) {
			perror("malloc");
			exit(EXIT_FAILURE);
		}
		new->id = ta->thread_id * 1000 + i;
		pthread_mutex_init(&new->lock, NULL);

		/*
		 * Find a position: for demo, we always insert after head.
		 * In a real scenario, threads would insert at different positions.
		 */
		struct list_node *pred = &list_head;
		struct list_node *succ = list_head.next;

		if (ta->use_fragile) {
			list_insert_fragile(new, pred, succ);
		} else {
			list_insert_robust(new, pred, succ);
		}

		pthread_mutex_lock(&node_count_lock);
		node_count++;
		pthread_mutex_unlock(&node_count_lock);
	}

	return NULL;
}

static void run_test(const char *name, int use_fragile, int nthreads,
		     int inserts_per_thread)
{
	pthread_t *threads;
	struct thread_arg *args;

	list_init(&list_head);
	node_count = 0;

	threads = malloc(sizeof(*threads) * nthreads);
	args = malloc(sizeof(*args) * nthreads);
	if (!threads || !args) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	printf("\n=== Test: %s ===\n", name);
	printf("Threads: %d, Inserts per thread: %d\n", nthreads,
	       inserts_per_thread);
	fflush(stdout);

	for (int i = 0; i < nthreads; i++) {
		args[i].thread_id = i;
		args[i].insert_count = inserts_per_thread;
		args[i].use_fragile = use_fragile;
		pthread_create(&threads[i], NULL, inserter_thread, &args[i]);
	}

	for (int i = 0; i < nthreads; i++) {
		pthread_join(threads[i], NULL);
	}

	printf("Final node count: %d (expected: %d)\n", node_count,
	       nthreads * inserts_per_thread);

	/* Cleanup: free all nodes except head */
	struct list_node *cur = list_head.next;
	while (cur != &list_head) {
		struct list_node *next = cur->next;
		pthread_mutex_destroy(&cur->lock);
		free(cur);
		cur = next;
	}
	pthread_mutex_destroy(&list_head.lock);

	free(threads);
	free(args);
}

int main(int argc, char *argv[])
{
	(void)argc;
	(void)argv;

	printf("Lock Ordering Demo -- Chapter 16: Ease of Use\n");
	printf("=============================================\n");
	printf("\nThis demo compares two approaches to locking a\n");
	printf("circular linked list during concurrent insertion.\n");
	fflush(stdout);

	/* Test 1: Robust address-order locking */
	run_test("Robust address-order locking", 0, 4, 1000);

	/* Test 2: Fragile list-order locking (same thread count, works here) */
	run_test("Fragile list-order locking (well-formed)", 1, 4, 1000);

	/*
	 * Test 3: Fragile list-order locking with MORE threads than nodes.
	 * In the real "fragile" scenario from the chapter, each thread locks
	 * a consecutive pair.  If threads > nodes-1, the proof breaks.
	 * Here we simulate by having many threads race on few positions.
	 */
	run_test("Fragile list-order locking (high contention)", 1, 8, 500);

	printf("\n=== Summary ===\n");
	printf("The 'robust' method (address-order) works regardless of\n");
	printf("thread count or list size. The 'fragile' method relies on\n");
	printf("a very specific mathematical invariant that is easy to break.\n");
	printf("This illustrates why we 'shave the Mandelbrot set' and prefer\n");
	printf("simple, general algorithms over clever but fragile ones.\n");

	return 0;
}
