/*
 * sref_demo.c - Simple reference counting protected by an external lock.
 * Corresponds to perfbook Section 13.2.1 "Simple Counting" ("-" category).
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

struct sref {
	int refcount;
};

static pthread_mutex_t obj_lock = PTHREAD_MUTEX_INITIALIZER;

void sref_init(struct sref *sref)
{
	sref->refcount = 1;
}

void sref_get(struct sref *sref)
{
	/* Called under obj_lock */
	sref->refcount++;
}

int sref_put(struct sref *sref, void (*release)(struct sref *sref))
{
	/* Called under obj_lock */
	if (--sref->refcount == 0) {
		release(sref);
		return 1;
	}
	return 0;
}

/* --- test infrastructure --- */
static int freed = 0;

void my_release(struct sref *sref)
{
	(void)sref;
	freed = 1;
	printf("  object released (refcount reached zero)\n");
}

int main(void)
{
	struct sref ref;

	printf("[sref_demo] Simple counting under a single lock\n");

	sref_init(&ref);
	printf("init -> refcount = %d\n", ref.refcount);

	pthread_mutex_lock(&obj_lock);
	sref_get(&ref);
	printf("get  -> refcount = %d\n", ref.refcount);

	sref_get(&ref);
	printf("get  -> refcount = %d\n", ref.refcount);

	sref_put(&ref, my_release);
	printf("put  -> refcount = %d (freed=%d)\n", ref.refcount, freed);

	sref_put(&ref, my_release);
	printf("put  -> refcount = %d (freed=%d)\n", ref.refcount, freed);

	sref_put(&ref, my_release);
	printf("put  -> refcount = %d (freed=%d)\n", ref.refcount, freed);
	pthread_mutex_unlock(&obj_lock);

	if (freed)
		printf("[OK] Simple reference counting works\n");
	else
		printf("[FAIL] Object was not freed\n");
	return freed ? 0 : 1;
}
