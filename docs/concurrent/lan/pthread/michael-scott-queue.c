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
//  Michael-Scott Lock-Free Queue
//
//  Based on: M. Michael & M. Scott,
//  "Simple, Fast, and Practical Non-Blocking and Blocking
//  Concurrent Queue Algorithms", PODC 1996.
// ------------------------------------------------------------------

typedef struct Node {
	_Atomic(struct Node *) next;
	void *data;
} Node;

typedef struct {
	_Atomic(Node *) head;
	_Atomic(Node *) tail;
} Queue;

static Queue g_queue;

// ------------------------------------------------------------------
//  队列操作
// ------------------------------------------------------------------
static void queue_init(void)
{
	Node *dummy = malloc(sizeof(Node));
	if (!dummy) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	__atomic_store_n(&dummy->next, NULL, __ATOMIC_SEQ_CST);
	dummy->data = NULL;

	__atomic_store_n(&g_queue.head, dummy, __ATOMIC_SEQ_CST);
	__atomic_store_n(&g_queue.tail, dummy, __ATOMIC_SEQ_CST);
}

static void enqueue(void *data)
{
	Node *node = malloc(sizeof(Node));
	if (!node) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}
	node->data = data;
	__atomic_store_n(&node->next, NULL, __ATOMIC_SEQ_CST);

	while (1) {
		Node *tail = __atomic_load_n(&g_queue.tail, __ATOMIC_SEQ_CST);
		Node *next = __atomic_load_n(&tail->next, __ATOMIC_SEQ_CST);

		// 确认 tail 未变
		if (tail == __atomic_load_n(&g_queue.tail, __ATOMIC_SEQ_CST)) {
			if (next == NULL) {
				// 尝试将新节点链接到 tail->next
				if (__atomic_compare_exchange_n(&tail->next, &next, node,
								false,
								__ATOMIC_SEQ_CST,
								__ATOMIC_SEQ_CST)) {
					// 尝试推进 tail（允许失败，由其他线程推进）
					__atomic_compare_exchange_n(&g_queue.tail, &tail, node,
								    false,
								    __ATOMIC_SEQ_CST,
								    __ATOMIC_SEQ_CST);
					return;
				}
			} else {
				// tail 滞后，尝试推进 tail
				__atomic_compare_exchange_n(&g_queue.tail, &tail, next,
							    false,
							    __ATOMIC_SEQ_CST,
							    __ATOMIC_SEQ_CST);
			}
		}
	}
}

static void *dequeue(void)
{
	while (1) {
		Node *head = __atomic_load_n(&g_queue.head, __ATOMIC_SEQ_CST);
		Node *tail = __atomic_load_n(&g_queue.tail, __ATOMIC_SEQ_CST);
		Node *next = __atomic_load_n(&head->next, __ATOMIC_SEQ_CST);

		// 确认 head 未变
		if (head == __atomic_load_n(&g_queue.head, __ATOMIC_SEQ_CST)) {
			if (head == tail) {
				if (next == NULL)
					return NULL;   // 空队列
				// tail 滞后，推进 tail
				__atomic_compare_exchange_n(&g_queue.tail, &tail, next,
							    false,
							    __ATOMIC_SEQ_CST,
							    __ATOMIC_SEQ_CST);
			} else {
				void *data = next->data;
				if (__atomic_compare_exchange_n(&g_queue.head, &head, next,
								false,
								__ATOMIC_SEQ_CST,
								__ATOMIC_SEQ_CST)) {
					return data;
				}
			}
		}
	}
}

// ------------------------------------------------------------------
//  测试
// ------------------------------------------------------------------
#define N_THREADS   8
#define N_OPS       100000

static pthread_barrier_t barrier;
static _Atomic long total_enq = 0;
static _Atomic long total_deq = 0;

void *worker(void *arg)
{
	long tid = (long)arg;
	unsigned int seed = (unsigned int)(time(NULL) ^ tid);

	pthread_barrier_wait(&barrier);

	for (int i = 0; i < N_OPS; i++) {
		int op = rand_r(&seed) % 2;   // 0 = enqueue, 1 = dequeue
		if (op == 0) {
			int *val = malloc(sizeof(int));
			*val = (int)(tid * N_OPS + i);
			enqueue(val);
			__atomic_fetch_add(&total_enq, 1, __ATOMIC_SEQ_CST);
		} else {
			void *val = dequeue();
			(void)val;   // data 由主线程统一释放，避免和清理循环 double free
			__atomic_fetch_add(&total_deq, 1, __ATOMIC_SEQ_CST);
		}
	}
	return NULL;
}

int main(void)
{
	srand((unsigned)time(NULL));

	queue_init();

	pthread_t threads[N_THREADS];
	pthread_barrier_init(&barrier, NULL, N_THREADS);

	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);

	for (long i = 0; i < N_THREADS; i++)
		pthread_create(&threads[i], NULL, worker, (void *)i);

	for (int i = 0; i < N_THREADS; i++)
		pthread_join(threads[i], NULL);

	clock_gettime(CLOCK_MONOTONIC, &end);

	double elapsed = (end.tv_sec - start.tv_sec)
			 + (end.tv_nsec - start.tv_nsec) / 1e9;

	printf("Threads: %d, Ops/thread: %d\n", N_THREADS, N_OPS);
	printf("Total enqueue: %ld\n", total_enq);
	printf("Total dequeue (success): %ld\n", total_deq);
	printf("Time: %.3f sec\n", elapsed);
	printf("Throughput: %.0f ops/sec\n",
	       (double)(N_THREADS * N_OPS) / elapsed);

	// 清理队列中剩余节点
	Node *cur = __atomic_load_n(&g_queue.head, __ATOMIC_SEQ_CST);
	while (cur) {
		Node *next = __atomic_load_n(&cur->next, __ATOMIC_SEQ_CST);
		if (cur->data)
			free(cur->data);
		free(cur);
		cur = next;
	}
	pthread_barrier_destroy(&barrier);
	return 0;
}
