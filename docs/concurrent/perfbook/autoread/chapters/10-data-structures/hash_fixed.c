// hash_fixed.c: Fixed-size hash table with per-bucket pthread_mutex_t.
// Demonstrates partitionable data structures for concurrent access.

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include <assert.h>

#define NBUCKETS 1024
#define NITEMS   100000
#define NREADERS 4
#define NWRITERS 2

struct ht_elem {
    struct ht_elem *next;
    unsigned long hash;
    int key;
    int value;
};

struct ht_bucket {
    struct ht_elem *head;
    pthread_mutex_t lock;
};

struct hashtab {
    unsigned long nbuckets;
    struct ht_bucket bkt[];
};

static struct hashtab *ht;

static struct hashtab *hashtab_alloc(unsigned long nbuckets)
{
    struct hashtab *htp = calloc(1, sizeof(*htp) + nbuckets * sizeof(struct ht_bucket));
    if (!htp) return NULL;
    htp->nbuckets = nbuckets;
    for (unsigned long i = 0; i < nbuckets; i++) {
        pthread_mutex_init(&htp->bkt[i].lock, NULL);
    }
    return htp;
}

static void hashtab_free(struct hashtab *htp)
{
    for (unsigned long i = 0; i < htp->nbuckets; i++) {
        struct ht_elem *e = htp->bkt[i].head;
        while (e) {
            struct ht_elem *next = e->next;
            free(e);
            e = next;
        }
        pthread_mutex_destroy(&htp->bkt[i].lock);
    }
    free(htp);
}

static unsigned long hashfn(int key)
{
    return (unsigned long)key;
}

static struct ht_elem *hashtab_lookup(struct hashtab *htp, int key)
{
    unsigned long h = hashfn(key);
    struct ht_bucket *b = &htp->bkt[h % htp->nbuckets];
    pthread_mutex_lock(&b->lock);
    struct ht_elem *e;
    for (e = b->head; e; e = e->next) {
        if (e->key == key) {
            pthread_mutex_unlock(&b->lock);
            return e;
        }
    }
    pthread_mutex_unlock(&b->lock);
    return NULL;
}

static void hashtab_add(struct hashtab *htp, int key, int value)
{
    unsigned long h = hashfn(key);
    struct ht_bucket *b = &htp->bkt[h % htp->nbuckets];
    struct ht_elem *e = malloc(sizeof(*e));
    assert(e);
    e->hash = h;
    e->key = key;
    e->value = value;
    pthread_mutex_lock(&b->lock);
    e->next = b->head;
    b->head = e;
    pthread_mutex_unlock(&b->lock);
}

static void hashtab_del(struct hashtab *htp, int key)
{
    unsigned long h = hashfn(key);
    struct ht_bucket *b = &htp->bkt[h % htp->nbuckets];
    pthread_mutex_lock(&b->lock);
    struct ht_elem **pp = &b->head;
    struct ht_elem *e;
    for (e = b->head; e; e = e->next) {
        if (e->key == key) {
            *pp = e->next;
            free(e);
            break;
        }
        pp = &e->next;
    }
    pthread_mutex_unlock(&b->lock);
}

static void *reader_thread(void *arg)
{
    (void)arg;
    for (int i = 0; i < NITEMS; i++) {
        struct ht_elem *e = hashtab_lookup(ht, i);
        if (e) {
            assert(e->value == i * 2);
        }
    }
    return NULL;
}

static void *writer_thread(void *arg)
{
    (void)arg;
    for (int i = 0; i < NITEMS; i++) {
        hashtab_add(ht, i, i * 2);
    }
    return NULL;
}

int main(void)
{
    ht = hashtab_alloc(NBUCKETS);
    assert(ht);

    pthread_t readers[NREADERS];
    pthread_t writers[NWRITERS];

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int i = 0; i < NWRITERS; i++)
        pthread_create(&writers[i], NULL, writer_thread, NULL);
    for (int i = 0; i < NWRITERS; i++)
        pthread_join(writers[i], NULL);

    for (int i = 0; i < NREADERS; i++)
        pthread_create(&readers[i], NULL, reader_thread, NULL);
    for (int i = 0; i < NREADERS; i++)
        pthread_join(readers[i], NULL);

    for (int i = 0; i < NITEMS; i++)
        hashtab_del(ht, i);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double dt = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) * 1e-9;
    printf("fixed-hash: %.3f sec (readers=%d writers=%d items=%d buckets=%d)\n",
           dt, NREADERS, NWRITERS, NITEMS, NBUCKETS);

    hashtab_free(ht);
    return 0;
}
