/*
 * hash_table.c: Demonstrate different locking granularities for hash table.
 *
 * This implements the variants from Chapter 6 of perfbook:
 * 1. Sequential (no locking)
 * 2. Code locking (global lock)
 * 3. Data locking (per-bucket lock)
 * 4. Reader/Writer locking (global rwlock)
 *
 * Build: make hash_table.out
 * Run:   ./hash_table.out [variant] [nthreads] [nops]
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>
#include <assert.h>

/* ---------- Shared data structures ---------- */

struct node {
	unsigned long key;
	struct node *next;
};

struct hash_table {
	long nbuckets;
	struct node **buckets;
};

/* ---------- Helper: simple sequential hash table ---------- */

static struct hash_table *ht_create(long nbuckets)
{
	struct hash_table *h = calloc(1, sizeof(*h));
	h->nbuckets = nbuckets;
	h->buckets = calloc(nbuckets, sizeof(struct node *));
	return h;
}

static void ht_insert(struct hash_table *h, unsigned long key)
{
	long b = key % h->nbuckets;
	struct node *n = malloc(sizeof(*n));
	n->key = key;
	n->next = h->buckets[b];
	h->buckets[b] = n;
}

/* Sequential search */
static int hash_search_seq(struct hash_table *h, unsigned long key)
{
	struct node *cur = h->buckets[key % h->nbuckets];
	while (cur != NULL) {
		if (cur->key >= key) {
			return (cur->key == key);
		}
		cur = cur->next;
	}
	return 0;
}

/* ---------- Variant 1: Code Locking (global mutex) ---------- */

static pthread_mutex_t code_lock = PTHREAD_MUTEX_INITIALIZER;

static int hash_search_code(struct hash_table *h, unsigned long key)
{
	struct node *cur;
	int retval;

	pthread_mutex_lock(&code_lock);
	cur = h->buckets[key % h->nbuckets];
	while (cur != NULL) {
		if (cur->key >= key) {
			retval = (cur->key == key);
			pthread_mutex_unlock(&code_lock);
			return retval;
		}
		cur = cur->next;
	}
	pthread_mutex_unlock(&code_lock);
	return 0;
}

/* ---------- Variant 2: Data Locking (per-bucket mutex) ---------- */

struct bucket {
	pthread_mutex_t lock;
	struct node *list_head;
};

struct hash_table_data {
	long nbuckets;
	struct bucket **buckets;
};

static struct hash_table_data *ht_data_create(long nbuckets)
{
	struct hash_table_data *h = calloc(1, sizeof(*h));
	h->nbuckets = nbuckets;
	h->buckets = calloc(nbuckets, sizeof(struct bucket *));
	for (long i = 0; i < nbuckets; i++) {
		h->buckets[i] = calloc(1, sizeof(struct bucket));
		pthread_mutex_init(&h->buckets[i]->lock, NULL);
	}
	return h;
}

static void ht_data_insert(struct hash_table_data *h, unsigned long key)
{
	long b = key % h->nbuckets;
	struct node *n = malloc(sizeof(*n));
	n->key = key;
	pthread_mutex_lock(&h->buckets[b]->lock);
	n->next = h->buckets[b]->list_head;
	h->buckets[b]->list_head = n;
	pthread_mutex_unlock(&h->buckets[b]->lock);
}

static int hash_search_data(struct hash_table_data *h, unsigned long key)
{
	struct bucket *bp;
	struct node *cur;
	int retval;

	bp = h->buckets[key % h->nbuckets];
	pthread_mutex_lock(&bp->lock);
	cur = bp->list_head;
	while (cur != NULL) {
		if (cur->key >= key) {
			retval = (cur->key == key);
			pthread_mutex_unlock(&bp->lock);
			return retval;
		}
		cur = cur->next;
	}
	pthread_mutex_unlock(&bp->lock);
	return 0;
}

/* ---------- Variant 3: Reader/Writer Locking ---------- */

static pthread_rwlock_t rw_lock = PTHREAD_RWLOCK_INITIALIZER;

static int hash_search_rw(struct hash_table *h, unsigned long key)
{
	struct node *cur;
	int retval;

	pthread_rwlock_rdlock(&rw_lock);
	cur = h->buckets[key % h->nbuckets];
	while (cur != NULL) {
		if (cur->key >= key) {
			retval = (cur->key == key);
			pthread_rwlock_unlock(&rw_lock);
			return retval;
		}
		cur = cur->next;
	}
	pthread_rwlock_unlock(&rw_lock);
	return 0;
}

/* ---------- Benchmark infrastructure ---------- */

static unsigned long g_nops = 1000000;
static int g_nthreads = 4;
static volatile int g_start = 0;

struct thread_arg {
	int id;
	void *ht;
	int (*search_fn)(void *, unsigned long);
	unsigned long ops;
	unsigned long found;
};

static void *worker(void *arg)
{
	struct thread_arg *a = arg;
	unsigned long seed = (unsigned long)a->id;
	unsigned long found = 0;

	while (!g_start)
		; /* spin-wait for start signal */

	for (unsigned long i = 0; i < a->ops; i++) {
		seed = seed * 1103515245 + 12345;
		unsigned long key = seed % (a->ops * 2);
		found += a->search_fn(a->ht, key);
	}

	a->found = found;
	return NULL;
}

static double now_sec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
}

static double run_benchmark(const char *name,
			    void *ht,
			    int (*search_fn)(void *, unsigned long))
{
	pthread_t *tids = calloc(g_nthreads, sizeof(pthread_t));
	struct thread_arg *args = calloc(g_nthreads, sizeof(struct thread_arg));
	unsigned long ops_per_thread = g_nops / g_nthreads;

	for (int i = 0; i < g_nthreads; i++) {
		args[i].id = i;
		args[i].ht = ht;
		args[i].search_fn = search_fn;
		args[i].ops = ops_per_thread;
		pthread_create(&tids[i], NULL, worker, &args[i]);
	}

	double t0 = now_sec();
	g_start = 1;
	for (int i = 0; i < g_nthreads; i++)
		pthread_join(tids[i], NULL);
	double t1 = now_sec();

	free(tids);
	free(args);
	g_start = 0;

	double elapsed = t1 - t0;
	printf("%-20s threads=%d ops=%lu time=%.4fs throughput=%.2f ops/sec\n",
	       name, g_nthreads, g_nops, elapsed, g_nops / elapsed);
	return elapsed;
}

int main(int argc, char *argv[])
{
	int variant = 0;
	if (argc > 1) variant = atoi(argv[1]);
	if (argc > 2) g_nthreads = atoi(argv[2]);
	if (argc > 3) g_nops = atol(argv[3]);

	printf("=== Hash Table Locking Granularity Benchmark ===\n");
	printf("variant=%d nthreads=%d nops=%lu\n\n", variant, g_nthreads, g_nops);

	long nbuckets = 1024;
	unsigned long ninsert = g_nops / 10;

	/* Sequential / Code-locking / RW-locking share same base structure */
	struct hash_table *ht = ht_create(nbuckets);
	for (unsigned long i = 0; i < ninsert; i++)
		ht_insert(ht, i * 2);

	/* Data-locking uses per-bucket structure */
	struct hash_table_data *ht_data = ht_data_create(nbuckets);
	for (unsigned long i = 0; i < ninsert; i++)
		ht_data_insert(ht_data, i * 2);

	if (variant == 0 || variant == 1) {
		printf("--- Sequential (no sync) ---\n");
		run_benchmark("sequential", ht,
			      (int (*)(void *, unsigned long))hash_search_seq);
	}

	if (variant == 0 || variant == 2) {
		printf("--- Code Locking (global mutex) ---\n");
		run_benchmark("code_lock", ht,
			      (int (*)(void *, unsigned long))hash_search_code);
	}

	if (variant == 0 || variant == 3) {
		printf("--- Data Locking (per-bucket mutex) ---\n");
		run_benchmark("data_lock", ht_data,
			      (int (*)(void *, unsigned long))hash_search_data);
	}

	if (variant == 0 || variant == 4) {
		printf("--- Reader/Writer Locking ---\n");
		run_benchmark("rw_lock", ht,
			      (int (*)(void *, unsigned long))hash_search_rw);
	}

	return 0;
}
