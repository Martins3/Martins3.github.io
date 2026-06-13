/*
 * seq_optimize.c
 * 演示顺序优化有时比并行化更有效
 *
 * 对应 perfbook 第 2 章 sec:intro:Performance Optimization
 * 核心观点：如果程序是 I/O  bound 或算法效率低，并行化可能无效甚至有害。
 * 用链表查找 vs 哈希表查找来说明。
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <pthread.h>

#define N_ELEMENTS 100000
#define N_LOOKUPS  100000
#define N_THREADS  4

/* 简单链表节点 */
struct node {
	int key;
	int value;
	struct node *next;
};

/* 简单哈希表（桶数组 + 链表） */
#define HASH_BITS 14
#define HASH_SIZE (1 << HASH_BITS)

struct hash_table {
	struct node *buckets[HASH_SIZE];
};

static uint64_t timespec_diff_ns(struct timespec *a, struct timespec *b)
{
	return (uint64_t)(b->tv_sec - a->tv_sec) * 1000000000ULL +
	       (uint64_t)(b->tv_nsec - a->tv_nsec);
}

static inline uint32_t hash_int(int key)
{
	/* 简单哈希函数 */
	return (uint32_t)key * 2654435761U;
}

/* 构建链表（头插法） */
static struct node *build_list(int n)
{
	struct node *head = NULL;
	for (int i = 0; i < n; i++) {
		struct node *n = malloc(sizeof(*n));
		n->key = i;
		n->value = i;
		n->next = head;
		head = n;
	}
	return head;
}

static void free_list(struct node *head)
{
	while (head) {
		struct node *next = head->next;
		free(head);
		head = next;
	}
}

/* 链表查找 */
static int list_lookup(struct node *head, int key)
{
	for (struct node *p = head; p; p = p->next)
		if (p->key == key)
			return p->value;
	return -1;
}

/* 构建哈希表 */
static struct hash_table *build_hash_table(int n)
{
	struct hash_table *ht = calloc(1, sizeof(*ht));
	for (int i = 0; i < n; i++) {
		struct node *node = malloc(sizeof(*node));
		node->key = i;
		node->value = i;
		uint32_t h = hash_int(i) & (HASH_SIZE - 1);
		node->next = ht->buckets[h];
		ht->buckets[h] = node;
	}
	return ht;
}

static void free_hash_table(struct hash_table *ht)
{
	for (int i = 0; i < HASH_SIZE; i++)
		free_list(ht->buckets[i]);
	free(ht);
}

/* 哈希表查找 */
static int hash_lookup(struct hash_table *ht, int key)
{
	uint32_t h = hash_int(key) & (HASH_SIZE - 1);
	for (struct node *p = ht->buckets[h]; p; p = p->next)
		if (p->key == key)
			return p->value;
	return -1;
}

/* 顺序链表查找 */
static double seq_list_lookup(struct node *list)
{
	struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int i = 0; i < N_LOOKUPS; i++) {
		int key = rand() % N_ELEMENTS;
		volatile int v = list_lookup(list, key);
		(void)v;
	}
	clock_gettime(CLOCK_MONOTONIC, &t1);
	return (double)timespec_diff_ns(&t0, &t1) / 1000000.0;
}

/* 顺序哈希表查找 */
static double seq_hash_lookup(struct hash_table *ht)
{
	struct timespec t0, t1;
	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int i = 0; i < N_LOOKUPS; i++) {
		int key = rand() % N_ELEMENTS;
		volatile int v = hash_lookup(ht, key);
		(void)v;
	}
	clock_gettime(CLOCK_MONOTONIC, &t1);
	return (double)timespec_diff_ns(&t0, &t1) / 1000000.0;
}

/* 并行链表查找参数 */
struct par_args {
	struct node *list;
	int thread_id;
};

static void *par_list_lookup(void *arg)
{
	struct par_args *p = arg;
	unsigned int seed = p->thread_id;
	int count = N_LOOKUPS / N_THREADS;
	for (int i = 0; i < count; i++) {
		int key = rand_r(&seed) % N_ELEMENTS;
		volatile int v = list_lookup(p->list, key);
		(void)v;
	}
	return NULL;
}

/* 并行哈希表查找 */
static void *par_hash_lookup(void *arg)
{
	struct par_args *p = arg;
	unsigned int seed = p->thread_id;
	int count = N_LOOKUPS / N_THREADS;
	for (int i = 0; i < count; i++) {
		int key = rand_r(&seed) % N_ELEMENTS;
		volatile int v = hash_lookup((struct hash_table *)p->list, key);
		(void)v;
	}
	return NULL;
}

static double parallel_lookup(void *(*fn)(void *), struct par_args *args)
{
	pthread_t threads[N_THREADS];
	struct timespec t0, t1;

	clock_gettime(CLOCK_MONOTONIC, &t0);
	for (int i = 0; i < N_THREADS; i++)
		pthread_create(&threads[i], NULL, fn, &args[i]);
	for (int i = 0; i < N_THREADS; i++)
		pthread_join(threads[i], NULL);
	clock_gettime(CLOCK_MONOTONIC, &t1);

	return (double)timespec_diff_ns(&t0, &t1) / 1000000.0;
}

int main(void)
{
	struct node *list;
	struct hash_table *ht;
	struct par_args args[N_THREADS];
	double t_seq_list, t_par_list, t_seq_hash, t_par_hash;

	printf("Sequential Optimization vs Parallelization Demo\n");
	printf("================================================\n");
	printf("elements=%d, lookups=%d, threads=%d\n\n",
	       N_ELEMENTS, N_LOOKUPS, N_THREADS);

	srand(time(NULL));

	/* 构建数据结构 */
	list = build_list(N_ELEMENTS);
	ht = build_hash_table(N_ELEMENTS);

	for (int i = 0; i < N_THREADS; i++) {
		args[i].thread_id = i;
	}

	/* 链表查找 */
	args[0].list = list;
	t_seq_list = seq_list_lookup(list);
	for (int i = 0; i < N_THREADS; i++)
		args[i].list = list;
	t_par_list = parallel_lookup(par_list_lookup, args);

	printf("Linked list lookup:\n");
	printf("  Sequential: %.3f ms\n", t_seq_list);
	printf("  Parallel:   %.3f ms (speedup: %.2fx)\n",
	       t_par_list, t_seq_list / t_par_list);

	/* 哈希表查找 */
	args[0].list = (struct node *)ht;
	t_seq_hash = seq_hash_lookup(ht);
	for (int i = 0; i < N_THREADS; i++)
		args[i].list = (struct node *)ht;
	t_par_hash = parallel_lookup(par_hash_lookup, args);

	printf("\nHash table lookup:\n");
	printf("  Sequential: %.3f ms\n", t_seq_hash);
	printf("  Parallel:   %.3f ms (speedup: %.2fx)\n",
	       t_par_hash, t_seq_hash / t_par_hash);

	printf("\nConclusion:\n");
	printf("  Sequential hash table is %.1fx faster than parallel linked list!\n",
	       t_par_list / t_seq_hash);
	printf("  Algorithm choice often dominates parallelization benefits.\n");

	free_list(list);
	free_hash_table(ht);

	return 0;
}
