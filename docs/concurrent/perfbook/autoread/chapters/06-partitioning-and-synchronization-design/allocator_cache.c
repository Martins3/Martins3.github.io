/*
 * allocator_cache.c: Parallel fastpath memory allocator demonstration.
 *
 * This implements the resource allocator cache pattern from Chapter 6:
 * - Each thread owns a modest cache of blocks (data ownership fastpath)
 * - A global code-locked pool handles overflow/underflow (slowpath)
 *
 * Build: make allocator_cache.out
 * Run:   ./allocator_cache.out [nthreads] [runlength]
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdint.h>

#define TARGET_POOL_SIZE 3
#define GLOBAL_POOL_SIZE 40
#define MAX_RUN 100
#define MAX_THREADS 64

struct memblock {
	char payload[64];
};

struct globalmempool {
	pthread_mutex_t mutex;
	int cur;
	struct memblock *pool[GLOBAL_POOL_SIZE];
};

struct perthreadmempool {
	int cur;
	struct memblock *pool[2 * TARGET_POOL_SIZE];
};

static struct globalmempool globalmem;
static __thread struct perthreadmempool perthreadmem;

void init_pools(void)
{
	static struct memblock memblocks[GLOBAL_POOL_SIZE];
	pthread_mutex_init(&globalmem.mutex, NULL);
	globalmem.cur = -1;
	perthreadmem.cur = -1;

	/* Seed global pool */
	for (int i = 0; i < GLOBAL_POOL_SIZE; i++) {
		globalmem.pool[++globalmem.cur] = &memblocks[i];
	}
}

/* Fastpath: no lock if local pool has blocks */
struct memblock *memblock_alloc(void)
{
	int i;
	struct memblock *p;
	struct perthreadmempool *pcpp = &perthreadmem;

	if (pcpp->cur < 0) {
		pthread_mutex_lock(&globalmem.mutex);
		for (i = 0; i < TARGET_POOL_SIZE && globalmem.cur >= 0; i++) {
			pcpp->pool[i] = globalmem.pool[globalmem.cur];
			globalmem.pool[globalmem.cur--] = NULL;
		}
		pcpp->cur = i - 1;
		pthread_mutex_unlock(&globalmem.mutex);
	}
	if (pcpp->cur >= 0) {
		p = pcpp->pool[pcpp->cur];
		pcpp->pool[pcpp->cur--] = NULL;
		return p;
	}
	return NULL;
}

/* Fastpath: no lock if local pool has room */
void memblock_free(struct memblock *p)
{
	int i;
	struct perthreadmempool *pcpp = &perthreadmem;

	if (pcpp->cur >= 2 * TARGET_POOL_SIZE - 1) {
		pthread_mutex_lock(&globalmem.mutex);
		for (i = pcpp->cur; i >= TARGET_POOL_SIZE; i--) {
			globalmem.pool[++globalmem.cur] = pcpp->pool[i];
			pcpp->pool[i] = NULL;
		}
		pcpp->cur = i;
		pthread_mutex_unlock(&globalmem.mutex);
	}
	pcpp->pool[++pcpp->cur] = p;
}

/* ---------- Benchmark ---------- */

static volatile int g_start = 0;
static volatile int g_stop = 0;

struct thread_arg {
	int id;
	int runlength;
	unsigned long ops;
	unsigned long failures;
};

static void *worker(void *arg)
{
	struct thread_arg *a = arg;
	struct memblock *p[MAX_RUN];
	unsigned long cnt = 0;
	unsigned long cntfail = 0;

	/* Initialize per-thread pool state */
	perthreadmem.cur = -1;

	while (!g_start)
		;

	while (!g_stop) {
		for (int i = 0; i < a->runlength; i++)
			p[i] = memblock_alloc();
		for (int i = 0; i < a->runlength; i++) {
			if (p[i] == NULL) {
				cntfail++;
			} else {
				memblock_free(p[i]);
				cnt++;
			}
		}
	}

	a->ops = cnt;
	a->failures = cntfail;
	return NULL;
}

static double now_sec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec / 1e9;
}

int main(int argc, char *argv[])
{
	int nthreads = 2;
	int runlength = 1;
	if (argc > 1) nthreads = atoi(argv[1]);
	if (argc > 2) runlength = atoi(argv[2]);
	if (runlength > MAX_RUN) runlength = MAX_RUN;

	printf("=== Allocator Cache Benchmark ===\n");
	printf("nthreads=%d runlength=%d target_pool=%d global_pool=%d\n\n",
	       nthreads, runlength, TARGET_POOL_SIZE, GLOBAL_POOL_SIZE);

	init_pools();

	pthread_t tids[MAX_THREADS];
	struct thread_arg args[MAX_THREADS];

	for (int i = 0; i < nthreads; i++) {
		args[i].id = i;
		args[i].runlength = runlength;
		args[i].ops = 0;
		args[i].failures = 0;
		pthread_create(&tids[i], NULL, worker, &args[i]);
	}

	g_start = 1;
	double t0 = now_sec();
	struct timespec ts = { .tv_sec = 1, .tv_nsec = 0 };
	nanosleep(&ts, NULL);
	g_stop = 1;
	double t1 = now_sec();

	unsigned long total_ops = 0;
	unsigned long total_fail = 0;
	for (int i = 0; i < nthreads; i++) {
		pthread_join(tids[i], NULL);
		total_ops += args[i].ops;
		total_fail += args[i].failures;
	}

	printf("time=%.3fs total_alloc_free_pairs=%lu failures=%lu throughput=%.2f ops/sec\n",
	       t1 - t0, total_ops, total_fail, total_ops / (t1 - t0));
	return 0;
}
