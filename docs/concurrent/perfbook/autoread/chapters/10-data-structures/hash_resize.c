// hash_resize.c: Simplified resizable hash table using indirection.
// Readers use lock-free traversal; writers lock both old and new buckets
// during resize. This demonstrates non-partitionable data structures.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <stdatomic.h>

#define NITEMS   50000
#define NREADERS 4

struct ht_elem {
    _Atomic(struct ht_elem *) next;
    int key;
    int value;
};

struct ht_bucket {
    _Atomic(struct ht_elem *) head;
    pthread_mutex_t lock;
};

struct ht {
    unsigned long nbuckets;
    struct ht_bucket bkt[];
};

struct hashtab {
    _Atomic(struct ht *) cur;
    pthread_mutex_t resize_lock;
};

static struct hashtab *ht_master;

static struct ht *ht_alloc(unsigned long nbuckets)
{
    struct ht *htp = calloc(1, sizeof(*htp) + nbuckets * sizeof(struct ht_bucket));
    if (!htp) return NULL;
    htp->nbuckets = nbuckets;
    for (unsigned long i = 0; i < nbuckets; i++) {
        atomic_store(&htp->bkt[i].head, NULL);
        pthread_mutex_init(&htp->bkt[i].lock, NULL);
    }
    return htp;
}

static struct hashtab *hashtab_alloc(unsigned long nbuckets)
{
    struct hashtab *m = calloc(1, sizeof(*m));
    if (!m) return NULL;
    struct ht *htp = ht_alloc(nbuckets);
    if (!htp) { free(m); return NULL; }
    atomic_store(&m->cur, htp);
    pthread_mutex_init(&m->resize_lock, NULL);
    return m;
}

static unsigned long hashfn(int key)
{
    return (unsigned long)key;
}

static struct ht_elem *hashtab_lookup(struct hashtab *m, int key)
{
    unsigned long h = hashfn(key);
    struct ht *htp = atomic_load_explicit(&m->cur, memory_order_acquire);
    struct ht_bucket *b = &htp->bkt[h % htp->nbuckets];
    struct ht_elem *e = atomic_load_explicit(&b->head, memory_order_acquire);
    while (e) {
        if (e->key == key)
            return e;
        e = atomic_load_explicit(&e->next, memory_order_acquire);
    }
    return NULL;
}

static void hashtab_add_to_ht(struct ht *htp, int key, int value)
{
    unsigned long h = hashfn(key);
    struct ht_bucket *b = &htp->bkt[h % htp->nbuckets];
    struct ht_elem *e = malloc(sizeof(*e));
    assert(e);
    e->key = key;
    e->value = value;
    struct ht_elem *old = atomic_load_explicit(&b->head, memory_order_relaxed);
    atomic_store_explicit(&e->next, old, memory_order_relaxed);
    atomic_thread_fence(memory_order_release);
    atomic_store_explicit(&b->head, e, memory_order_release);
}

static void hashtab_add(struct hashtab *m, int key, int value)
{
    struct ht *htp = atomic_load_explicit(&m->cur, memory_order_acquire);
    unsigned long h = hashfn(key);
    struct ht_bucket *b = &htp->bkt[h % htp->nbuckets];
    pthread_mutex_lock(&b->lock);
    hashtab_add_to_ht(htp, key, value);
    pthread_mutex_unlock(&b->lock);
}

static void hashtab_resize(struct hashtab *m, unsigned long new_nbuckets)
{
    if (pthread_mutex_trylock(&m->resize_lock) != 0) {
        printf("resize already in progress\n");
        return;
    }
    struct ht *old_ht = atomic_load_explicit(&m->cur, memory_order_relaxed);
    struct ht *new_ht = ht_alloc(new_nbuckets);
    if (!new_ht) {
        pthread_mutex_unlock(&m->resize_lock);
        return;
    }
    // Migrate elements
    for (unsigned long i = 0; i < old_ht->nbuckets; i++) {
        pthread_mutex_lock(&old_ht->bkt[i].lock);
        struct ht_elem *e = atomic_load_explicit(&old_ht->bkt[i].head, memory_order_relaxed);
        while (e) {
            hashtab_add_to_ht(new_ht, e->key, e->value);
            e = atomic_load_explicit(&e->next, memory_order_relaxed);
        }
        pthread_mutex_unlock(&old_ht->bkt[i].lock);
    }
    atomic_store_explicit(&m->cur, new_ht, memory_order_release);
    pthread_mutex_unlock(&m->resize_lock);

    // Free old table elements and buckets
    for (unsigned long i = 0; i < old_ht->nbuckets; i++) {
        struct ht_elem *e = atomic_load_explicit(&old_ht->bkt[i].head, memory_order_relaxed);
        while (e) {
            struct ht_elem *next = atomic_load_explicit(&e->next, memory_order_relaxed);
            free(e);
            e = next;
        }
        pthread_mutex_destroy(&old_ht->bkt[i].lock);
    }
    free(old_ht);
}

static void *reader_thread(void *arg)
{
    (void)arg;
    for (int i = 0; i < NITEMS; i++) {
        struct ht_elem *e = hashtab_lookup(ht_master, i);
        if (e) {
            assert(e->value == i * 2);
        }
    }
    return NULL;
}

int main(void)
{
    ht_master = hashtab_alloc(256);
    assert(ht_master);

    // Populate
    for (int i = 0; i < NITEMS; i++)
        hashtab_add(ht_master, i, i * 2);

    pthread_t readers[NREADERS];
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int i = 0; i < NREADERS; i++)
        pthread_create(&readers[i], NULL, reader_thread, NULL);

    // Resize concurrently with readers
    hashtab_resize(ht_master, 2048);

    for (int i = 0; i < NREADERS; i++)
        pthread_join(readers[i], NULL);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double dt = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) * 1e-9;
    printf("resize-hash: %.3f sec (readers=%d items=%d resized 256->2048)\n",
           dt, NREADERS, NITEMS);

    // Cleanup (simplified: just leak for this demo, or properly free)
    struct ht *htp = atomic_load(&ht_master->cur);
    for (unsigned long i = 0; i < htp->nbuckets; i++) {
        struct ht_elem *e = atomic_load_explicit(&htp->bkt[i].head, memory_order_relaxed);
        while (e) {
            struct ht_elem *next = atomic_load_explicit(&e->next, memory_order_relaxed);
            free(e);
            e = next;
        }
        pthread_mutex_destroy(&htp->bkt[i].lock);
    }
    free(htp);
    pthread_mutex_destroy(&ht_master->resize_lock);
    free(ht_master);
    return 0;
}
