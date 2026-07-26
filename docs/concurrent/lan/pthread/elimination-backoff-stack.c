#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <time.h>

// ------------------------------------------------------------------
//  Elimination-Backoff Stack
//
//  Based on: Hendler, Shavit, Yerushalmi, "A Scalable Lock-free Stack
//  Algorithm", SPAA 2004.
//
//  Simplification: instead of the complex location+collision protocol
//  from the paper, we use a single atomic elimination array where
//  pushes deposit cells and pops try to snatch them.  This keeps the
//  code small and correct while preserving the core idea (elimination
//  as a backoff scheme).
// ------------------------------------------------------------------

typedef struct Cell {
	struct Cell *pnext;
	void *pdata;
} Cell;

typedef struct {
	_Atomic(Cell *) ptop;
} Stack;

static Stack g_stack;
static _Atomic(Cell *) *g_elim;
static int g_elim_size;

// ------------------------------------------------------------------
//  Treiber 栈操作
// ------------------------------------------------------------------
//
static bool try_push(Cell *cell)
{
	Cell *phead = __atomic_load_n(&g_stack.ptop, __ATOMIC_SEQ_CST);
	cell->pnext = phead;
	return __atomic_compare_exchange_n(&g_stack.ptop, &phead, cell, false,
					   __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

static Cell *try_pop(void)
{
	Cell *phead = __atomic_load_n(&g_stack.ptop, __ATOMIC_SEQ_CST);
	if (phead == NULL)
		return NULL;
	Cell *pnext = phead->pnext;
	if (__atomic_compare_exchange_n(&g_stack.ptop, &phead, pnext, false,
					__ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
		return phead;
	return (Cell *)-1; // CAS 失败，需要外层重试
}

static inline void delay(int spin)
{
	for (volatile int i = 0; i < spin; i++)
		__asm__ volatile("nop");
}

// ------------------------------------------------------------------
//  公共 API
// ------------------------------------------------------------------
void ebstack_init(int elim_size)
{
	__atomic_store_n(&g_stack.ptop, NULL, __ATOMIC_SEQ_CST);
	g_elim_size = elim_size;
	g_elim = calloc(elim_size, sizeof(_Atomic(Cell *)));
}

void ebstack_push(int tid, void *data, unsigned int *seed)
{
	(void)tid;
	Cell *cell = malloc(sizeof(Cell));
	if (!cell) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	cell->pdata = data;
	cell->pnext = NULL;

	while (1) {
		if (try_push(cell))
			return;

		// 退避到 elimination 数组
		int pos = rand_r(seed) % g_elim_size;
		Cell *empty = NULL;
		if (__atomic_compare_exchange_n(&g_elim[pos], &empty, cell,
						false, __ATOMIC_SEQ_CST,
						__ATOMIC_SEQ_CST)) {
			delay(200);
			Cell *cur =
				__atomic_load_n(&g_elim[pos], __ATOMIC_SEQ_CST);
			if (cur == cell) {
				// 尝试撤销：用 CAS 确保 pop 没有同时取走
				if (!__atomic_compare_exchange_n(
					    &g_elim[pos], &cur, NULL, false,
					    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST))
					return; // pop 同时取走了，由 pop 释放 cell
			} else {
				// 已被 pop 取走，由 pop 释放 cell
				return;
			}
		}
		// slot 被占用，回到 while 开头重试 Treiber
	}
}

void *ebstack_pop(int tid, unsigned int *seed)
{
	(void)tid;
	while (1) {
		Cell *cell = try_pop();
		if (cell != (Cell *)-1) {
			if (cell) {
				void *data = cell->pdata;
				// cell 本身不在这里释放，避免 Treiber 中 use-after-free
				return data;
			}
			return NULL; // 空栈
		}

		// 退避到 elimination 数组
		int pos = rand_r(seed) % g_elim_size;
		Cell *pending = __atomic_load_n(&g_elim[pos], __ATOMIC_SEQ_CST);
		if (pending != NULL) {
			if (__atomic_compare_exchange_n(
				    &g_elim[pos], &pending, NULL, false,
				    __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST)) {
				void *data = pending->pdata;
				free(pending); // pop 取走了 elimination 中的 cell，负责释放
				return data;
			}
		}
	}
}

// ------------------------------------------------------------------
//  测试
// ------------------------------------------------------------------
#define N_THREADS 8
#define N_OPS 100000

static pthread_barrier_t barrier;
static _Atomic long total_push = 0;
static _Atomic long total_pop = 0;

void *worker(void *arg)
{
	long tid = (long)arg;
	unsigned int seed = (unsigned int)(time(NULL) ^ tid);

	pthread_barrier_wait(&barrier);

	for (int i = 0; i < N_OPS; i++) {
		int op = rand_r(&seed) % 2; // 0 = push, 1 = pop
		if (op == 0) {
			int *val = malloc(sizeof(int));
			*val = (int)(tid * N_OPS + i);
			ebstack_push((int)tid, val, &seed);
			__atomic_fetch_add(&total_push, 1, __ATOMIC_SEQ_CST);
		} else {
			void *val = ebstack_pop((int)tid, &seed);
			if (val)
				free(val);
			__atomic_fetch_add(&total_pop, 1, __ATOMIC_SEQ_CST);
		}
	}
	return NULL;
}

int main(void)
{
	srand((unsigned)time(NULL));

	ebstack_init(N_THREADS * 4);

	pthread_t threads[N_THREADS];
	pthread_barrier_init(&barrier, NULL, N_THREADS);

	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	for (long i = 0; i < N_THREADS; i++)
		pthread_create(&threads[i], NULL, worker, (void *)i);

	for (int i = 0; i < N_THREADS; i++)
		pthread_join(threads[i], NULL);

	clock_gettime(CLOCK_MONOTONIC, &end);

	double elapsed = (end.tv_sec - start.tv_sec) +
			 (end.tv_nsec - start.tv_nsec) / 1e9;

	printf("Threads: %d, Ops/thread: %d\n", N_THREADS, N_OPS);
	printf("Total push: %ld\n", total_push);
	printf("Total pop (success): %ld\n", total_pop);
	printf("Time: %.3f sec\n", elapsed);
	printf("Throughput: %.0f ops/sec\n",
	       (double)(N_THREADS * N_OPS) / elapsed);

	// 清理 Treiber 栈中剩余节点
	Cell *c = __atomic_load_n(&g_stack.ptop, __ATOMIC_SEQ_CST);
	while (c) {
		Cell *next = c->pnext;
		free(c->pdata);
		free(c);
		c = next;
	}
	// 清理 elimination 数组中残留的 cell
	for (int i = 0; i < g_elim_size; i++) {
		Cell *c = __atomic_load_n(&g_elim[i], __ATOMIC_SEQ_CST);
		if (c) {
			free(c->pdata);
			free(c);
		}
	}
	free(g_elim);
	pthread_barrier_destroy(&barrier);
	return 0;
}
