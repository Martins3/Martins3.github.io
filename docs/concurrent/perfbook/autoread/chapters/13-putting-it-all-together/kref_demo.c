/*
 * kref_demo.c - Atomic reference counting (Linux kref style).
 * Corresponds to perfbook Section 13.2.2 "Atomic Counting" ("A" category).
 *
 * In the real kernel kref_get() requires the caller already hold a reference,
 * so there is no check.  kref_get_unless_zero() is used when acquiring from
 * a lookup structure.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdatomic.h>

struct kref {
	atomic_int refcount;
};

void kref_init(struct kref *kref)
{
	atomic_init(&kref->refcount, 1);
}

void kref_get(struct kref *kref)
{
	/* Caller must already hold a reference. */
	atomic_fetch_add_explicit(&kref->refcount, 1, memory_order_relaxed);
}

int kref_get_unless_zero(struct kref *kref)
{
	int old = atomic_load_explicit(&kref->refcount, memory_order_relaxed);
	int new;
	do {
		if (old == 0)
			return 0;
		new = old + 1;
	} while (!atomic_compare_exchange_weak_explicit(
			&kref->refcount, &old, new,
			memory_order_acquire, memory_order_relaxed));
	return 1;
}

int kref_put(struct kref *kref, void (*release)(struct kref *kref))
{
	if (atomic_fetch_sub_explicit(&kref->refcount, 1, memory_order_release) == 1) {
		atomic_thread_fence(memory_order_acquire);
		release(kref);
		return 1;
	}
	return 0;
}

/* --- test --- */
static int freed = 0;

void my_release(struct kref *kref)
{
	(void)kref;
	freed = 1;
	printf("  object released\n");
}

int main(void)
{
	struct kref ref;

	printf("[kref_demo] Atomic counting (Linux kref style)\n");

	kref_init(&ref);
	printf("init -> refcount = %d\n", atomic_load(&ref.refcount));

	kref_get(&ref);
	printf("get  -> refcount = %d\n", atomic_load(&ref.refcount));

	if (kref_get_unless_zero(&ref))
		printf("get_unless_zero succeeded -> refcount = %d\n",
		       atomic_load(&ref.refcount));
	else
		printf("get_unless_zero failed\n");

	kref_put(&ref, my_release);
	printf("put  -> refcount = %d freed=%d\n",
	       atomic_load(&ref.refcount), freed);

	kref_put(&ref, my_release);
	printf("put  -> refcount = %d freed=%d\n",
	       atomic_load(&ref.refcount), freed);

	kref_put(&ref, my_release);
	printf("put  -> refcount = %d freed=%d\n",
	       atomic_load(&ref.refcount), freed);

	/* Now get_unless_zero must fail */
	if (!kref_get_unless_zero(&ref))
		printf("[OK] get_unless_zero correctly failed on zero refcount\n");
	else
		printf("[FAIL] get_unless_zero should have failed\n");

	return freed ? 0 : 1;
}
