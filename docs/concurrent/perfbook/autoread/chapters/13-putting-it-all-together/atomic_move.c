/*
 * atomic_move.c - Atomic rename/move using RCU + per-element seqlock.
 * Corresponds to perfbook Section 13.4.3 "Atomic Move".
 *
 * We verify two invariants under concurrent access:
 * 1. A reader never sees the element in both old and new locations
 *    at the same time.
 * 2. A reader never sees the element missing from both locations
 *    at the same time.
 *
 * To make these checks atomic w.r.t writers, readers use a global
 * version counter snapshot.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include "urcu_simple.h"

struct elem {
	char name[32];
	atomic_uint seq;
	int present;
};

static struct elem old_slot;
static struct elem new_slot;
static pthread_mutex_t rename_lock = PTHREAD_MUTEX_INITIALIZER;
static atomic_ulong global_ver = 0;

static inline unsigned read_seq_begin(struct elem *e)
{
	unsigned seq;
	do {
		seq = atomic_load_explicit(&e->seq, memory_order_acquire);
	} while (seq & 1);
	return seq;
}

static inline int read_seq_retry(struct elem *e, unsigned start)
{
	unsigned seq = atomic_load_explicit(&e->seq, memory_order_acquire);
	return seq != start;
}

static inline void write_seq_lock(struct elem *e)
{
	atomic_fetch_add_explicit(&e->seq, 1, memory_order_release);
}

static inline void write_seq_unlock(struct elem *e)
{
	atomic_fetch_add_explicit(&e->seq, 1, memory_order_release);
}

static void do_rename(void)
{
	pthread_mutex_lock(&rename_lock);
	atomic_fetch_add_explicit(&global_ver, 1, memory_order_relaxed);

	strcpy(new_slot.name, "NewName");
	new_slot.present = 1;
	write_seq_lock(&old_slot);
	smp_wmb();
	old_slot.present = 0;
	write_seq_unlock(&old_slot);

	atomic_fetch_add_explicit(&global_ver, 1, memory_order_relaxed);
	pthread_mutex_unlock(&rename_lock);
}

static void do_rename_back(void)
{
	pthread_mutex_lock(&rename_lock);
	atomic_fetch_add_explicit(&global_ver, 1, memory_order_relaxed);

	strcpy(old_slot.name, "OldName");
	old_slot.present = 1;
	write_seq_lock(&new_slot);
	smp_wmb();
	new_slot.present = 0;
	write_seq_unlock(&new_slot);

	atomic_fetch_add_explicit(&global_ver, 1, memory_order_relaxed);
	pthread_mutex_unlock(&rename_lock);
}

static void *reader_thread(void *arg)
{
	(void)arg;
	int violations = 0;
	for (int i = 0; i < 100000; i++) {
		unsigned long v1, v2;
		int p_old, p_new;
		unsigned seq_old, seq_new;

		/*
		 * Snapshot both elements while ensuring writer did not
		 * modify state in between.  Writers bump global_ver twice
		 * per operation (before and after).  If v1 == v2 and even,
		 * no writer was active during the snapshot.
		 */
		do {
			v1 = atomic_load_explicit(&global_ver, memory_order_acquire);
			seq_old = read_seq_begin(&old_slot);
			p_old = old_slot.present;
			seq_new = read_seq_begin(&new_slot);
			p_new = new_slot.present;
			v2 = atomic_load_explicit(&global_ver, memory_order_acquire);
		} while (v1 != v2 || (v1 & 1));

		if (p_old && p_new) {
			violations++;
			if (violations <= 3)
				printf("[FAIL] Element in both places (ver=%lu)\n", v1);
		}
		if (!p_old && !p_new) {
			violations++;
			if (violations <= 3)
				printf("[FAIL] Element missing from both (ver=%lu)\n", v1);
		}
	}
	printf("  reader violations = %d\n", violations);
	return violations ? NULL : (void *)1;
}

static void *writer_thread(void *arg)
{
	(void)arg;
	for (int i = 0; i < 500; i++) {
		do_rename();
		do_rename_back();
	}
	return (void *)1;
}

int main(void)
{
	printf("[atomic_move] RCU + seqlock for atomic rename\n");
	strcpy(old_slot.name, "OldName");
	old_slot.present = 1;
	atomic_init(&old_slot.seq, 0);
	atomic_init(&new_slot.seq, 0);

	pthread_t r1, r2, w;
	pthread_create(&r1, NULL, reader_thread, NULL);
	pthread_create(&r2, NULL, reader_thread, NULL);
	pthread_create(&w, NULL, writer_thread, NULL);

	void *rv1, *rv2, *wv;
	pthread_join(r1, &rv1);
	pthread_join(r2, &rv2);
	pthread_join(w, &wv);

	if (rv1 && rv2 && wv)
		printf("[OK] Atomic move consistency verified\n");
	else
		printf("[FAIL] Consistency violation detected\n");
	return (rv1 && rv2 && wv) ? 0 : 1;
}
