/*
 * seqlock_demo.c
 *
 * Demonstrate seqlock as a restricted form of STM (Software Transactional
 * Memory), as discussed in perfbook Chapter 17.
 *
 * Seqlock handles many STM challenges well:
 * - I/O operations: executed on each retry pass
 * - Process modifications: work normally
 * - Locking: can be used freely within read-side critical sections
 * - Contention management: readers always rolled back on conflict
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <stdatomic.h>

/* Simple seqlock implementation */
typedef struct {
    atomic_uint seq;
    pthread_mutex_t lock;
} seqlock_t;

static inline void seqlock_init(seqlock_t *sl)
{
    atomic_init(&sl->seq, 0);
    pthread_mutex_init(&sl->lock, NULL);
}

static inline unsigned seqlock_read_begin(seqlock_t *sl)
{
    unsigned seq;
    /* Wait for writer to finish (odd sequence means writer active) */
    do {
        seq = atomic_load_explicit(&sl->seq, memory_order_acquire);
    } while (seq & 1);
    return seq;
}

static inline int seqlock_read_retry(seqlock_t *sl, unsigned start)
{
    /* smp_rmb() equivalent */
    atomic_thread_fence(memory_order_seq_cst);
    return atomic_load_explicit(&sl->seq, memory_order_relaxed) != start;
}

static inline void seqlock_write_begin(seqlock_t *sl)
{
    pthread_mutex_lock(&sl->lock);
    unsigned seq = atomic_load_explicit(&sl->seq, memory_order_relaxed);
    atomic_store_explicit(&sl->seq, seq + 1, memory_order_release);
    atomic_thread_fence(memory_order_seq_cst);
}

static inline void seqlock_write_end(seqlock_t *sl)
{
    unsigned seq = atomic_load_explicit(&sl->seq, memory_order_relaxed);
    atomic_store_explicit(&sl->seq, seq + 1, memory_order_release);
    pthread_mutex_unlock(&sl->lock);
}

/* Protected data */
static seqlock_t g_seqlock;
static volatile uint64_t g_data[4];

#define NR_READERS 4
#define NR_WRITERS 2
#define ITERATIONS 1000000

static void *reader_thread(void *arg)
{
    (void)arg;
    uint64_t local[4];
    unsigned long retries = 0;

    for (int i = 0; i < ITERATIONS; i++) {
        unsigned seq;
        int retry;
        do {
            seq = seqlock_read_begin(&g_seqlock);
            /* Read-side critical section: can include I/O, delays, etc. */
            local[0] = g_data[0];
            local[1] = g_data[1];
            local[2] = g_data[2];
            local[3] = g_data[3];
            retry = seqlock_read_retry(&g_seqlock, seq);
            if (retry)
                retries++;
        } while (retry);

        /* Verify consistency: all fields should have same generation */
        if (local[0] != local[1] || local[1] != local[2] || local[2] != local[3]) {
            fprintf(stderr, "Inconsistent read!\n");
            exit(1);
        }
    }

    printf("Reader done, retries: %lu\n", retries);
    return NULL;
}

static void *writer_thread(void *arg)
{
    (void)arg;
    uint64_t val = (uint64_t)(size_t)arg;

    for (int i = 0; i < ITERATIONS; i++) {
        seqlock_write_begin(&g_seqlock);
        /* Write-side critical section: must be serialized */
        g_data[0] = val + i;
        g_data[1] = val + i;
        g_data[2] = val + i;
        g_data[3] = val + i;
        seqlock_write_end(&g_seqlock);
    }

    printf("Writer done\n");
    return NULL;
}

int main(void)
{
    pthread_t readers[NR_READERS];
    pthread_t writers[NR_WRITERS];

    seqlock_init(&g_seqlock);
    g_data[0] = g_data[1] = g_data[2] = g_data[3] = 0;

    printf("Starting seqlock demo (restricted STM)\n");
    printf("Readers: %d, Writers: %d, Iterations: %d\n",
           NR_READERS, NR_WRITERS, ITERATIONS);

    for (int i = 0; i < NR_READERS; i++)
        pthread_create(&readers[i], NULL, reader_thread, NULL);
    for (int i = 0; i < NR_WRITERS; i++)
        pthread_create(&writers[i], NULL, writer_thread,
                       (void *)(size_t)(i + 1));

    for (int i = 0; i < NR_READERS; i++)
        pthread_join(readers[i], NULL);
    for (int i = 0; i < NR_WRITERS; i++)
        pthread_join(writers[i], NULL);

    printf("All threads completed successfully\n");
    return 0;
}
