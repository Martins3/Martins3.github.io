/*
 * existence_guarantee.c: 基于锁的存在性保证
 *
 * 对应 perfbook 第7章:
 *  - 7.4 Lock-Based Existence Guarantees
 *
 * 演示为什么 "per-element locking" 不充分，
 * 以及如何使用全局/哈希锁提供存在性保证。
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define HASH_SIZE 16

/* ========== 有 BUG 的版本: 先获取元素锁，但元素可能被释放 ========== */

struct element_buggy {
	int key;
	int value;
	pthread_mutex_t lock;
};

struct element_buggy *hashtable_buggy[HASH_SIZE];
pthread_mutex_t global_buggy_lock = PTHREAD_MUTEX_INITIALIZER;

int hashfunction(int key)
{
	return key % HASH_SIZE;
}

/* 有问题的 delete: 先拿到指针，再锁元素，但其他线程可能在锁之前释放元素 */
int delete_buggy(int key)
{
	int b = hashfunction(key);
	struct element_buggy *p = hashtable_buggy[b];

	if (p == NULL || p->key != key)
		return 0;

	/* BUG: 此时 p 可能已经被另一个线程释放！ */
	pthread_mutex_lock(&p->lock);
	hashtable_buggy[b] = NULL;
	pthread_mutex_unlock(&p->lock);
	free(p);
	return 1;
}

/* ========== 正确的版本: 使用全局/桶锁保证存在性 ========== */

struct element {
	int key;
	int value;
};

struct element *hashtable[HASH_SIZE];
pthread_mutex_t locktable[HASH_SIZE]; /* 每个哈希桶一个锁 */

void init_hashtable(void)
{
	for (int i = 0; i < HASH_SIZE; i++) {
		hashtable[i] = NULL;
		pthread_mutex_init(&locktable[i], NULL);
	}
}

int delete_safe(int key)
{
	int b = hashfunction(key);
	pthread_mutex_t *sp = &locktable[b];
	struct element *p;

	pthread_mutex_lock(sp);   /* 先锁桶 */
	p = hashtable[b];         /* 再拿指针，此时元素一定存在 */
	if (p == NULL || p->key != key) {
		pthread_mutex_unlock(sp);
		return 0;
	}
	hashtable[b] = NULL;
	pthread_mutex_unlock(sp);
	free(p);                  /* 安全释放 */
	return 1;
}

/* ========== 并发测试 ========== */

#define NTHREADS 4
#define NOPS 10000

void *test_safe_delete(void *arg)
{
	(void)arg;
	for (int i = 0; i < NOPS; i++) {
		int key = i % HASH_SIZE;
		/* 随机插入或删除 */
		int b = hashfunction(key);
		pthread_mutex_lock(&locktable[b]);
		if (hashtable[b] == NULL) {
			struct element *e = malloc(sizeof(*e));
			e->key = key;
			e->value = i;
			hashtable[b] = e;
		}
		pthread_mutex_unlock(&locktable[b]);

		/* 尝试删除 */
		delete_safe(key);
	}
	return NULL;
}

int main(void)
{
	pthread_t threads[NTHREADS];

	printf("=== Lock-Based Existence Guarantees ===\n\n");

	init_hashtable();

	printf("--- Safe delete with bucket-level locking ---\n");
	for (int i = 0; i < NTHREADS; i++)
		pthread_create(&threads[i], NULL, test_safe_delete, NULL);
	for (int i = 0; i < NTHREADS; i++)
		pthread_join(threads[i], NULL);

	/* 清理 */
	for (int i = 0; i < HASH_SIZE; i++) {
		if (hashtable[i])
			free(hashtable[i]);
	}

	printf("[Existence] Test completed without crash.\n\n");

	printf("Key insight:\n");
	printf("  - BUG:  p = hashtable[b]; spin_lock(&p->lock); free(p);\n");
	printf("         Another thread might free 'p' between the first two lines.\n");
	printf("  - FIX:  spin_lock(&locktable[b]); p = hashtable[b]; ... free(p);\n");
	printf("         The bucket lock guarantees the element exists until unlock.\n");
	printf("  - For more complex structures (trees, multiple hash tables),\n");
	printf("    see RCU, reference counting, or hazard pointers (Chapter 13).\n");

	return 0;
}
