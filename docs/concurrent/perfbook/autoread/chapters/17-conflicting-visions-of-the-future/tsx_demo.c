/*
 * tsx_demo.c
 *
 * Demonstrate Intel TSX (Transactional Synchronization Extensions) as
 * a form of HTM (Hardware Transactional Memory).
 *
 * This code shows:
 * 1. Transaction begin/end using _xbegin()/_xend()
 * 2. Abort handling with fallback to locking
 * 3. The semantic difference between locking and HTM
 *
 * Note: Modern Intel CPUs (12th Gen+) have TSX disabled by default
 * due to security vulnerabilities (TAA, MDS). On such systems, _xbegin()
 * will always abort, and the fallback path will be taken.
 * The kernel handles this via arch/x86/kernel/cpu/tsx.c.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <immintrin.h>

/* TSX abort status bits */
#define XABORT_EXPLICIT  (1 << 0)
#define XABORT_RETRY     (1 << 1)
#define XABORT_CONFLICT  (1 << 2)
#define XABORT_CAPACITY  (1 << 3)
#define XABORT_DEBUG     (1 << 4)
#define XABORT_NESTED    (1 << 5)

/* Fallback lock */
static pthread_mutex_t g_fallback_lock = PTHREAD_MUTEX_INITIALIZER;

/* Shared data - a simple counter and metadata */
static uint64_t g_counter = 0;
static uint64_t g_metadata = 0;

/* Statistics */
static _Atomic uint64_t g_tx_commits = 0;
static _Atomic uint64_t g_tx_aborts = 0;
static _Atomic uint64_t g_fallback_count = 0;

#define NR_THREADS 4
#define ITERATIONS 100000

/*
 * Perform an atomic update using TSX with fallback to pthread_mutex.
 * This demonstrates "Transactional Lock Elision" (TLE) pattern.
 */
static void tsx_update(void)
{
    unsigned status;
    int retries = 5;

retry:
    if (--retries < 0) {
        /* Too many retries, use fallback lock */
        pthread_mutex_lock(&g_fallback_lock);
        g_counter++;
        g_metadata = g_counter * 2;
        pthread_mutex_unlock(&g_fallback_lock);
        g_fallback_count++;
        return;
    }

    status = _xbegin();
    if (status == _XBEGIN_STARTED) {
        /* Transaction started successfully */
        /*
         * In a real transaction, we read/write shared data.
         * The hardware tracks cache lines touched.
         */
        uint64_t old = g_counter;
        g_counter = old + 1;
        g_metadata = (old + 1) * 2;

        /*
         * Check if we need fallback (e.g., for debugging, I/O,
         * or irrevocable operations). For demo, we skip this.
         */

        _xend();
        g_tx_commits++;
        return;
    }

    /* Transaction aborted - analyze reason */
    g_tx_aborts++;

    if (status & XABORT_RETRY) {
        /* Transaction may succeed on retry */
        goto retry;
    }
    if (status & XABORT_CONFLICT) {
        /* Conflict with another transaction/thread */
        goto retry;
    }
    if (status & XABORT_CAPACITY) {
        /* Transaction too large for cache */
        goto fallback;
    }
    if (status & XABORT_EXPLICIT) {
        /* Explicit abort via _xabort() */
        goto fallback;
    }
    /* Unknown abort reason - fallback to lock */
    goto fallback;

fallback:
    pthread_mutex_lock(&g_fallback_lock);
    g_counter++;
    g_metadata = g_counter * 2;
    pthread_mutex_unlock(&g_fallback_lock);
    g_fallback_count++;
}

static void *worker_thread(void *arg)
{
    (void)arg;
    for (int i = 0; i < ITERATIONS; i++)
        tsx_update();
    return NULL;
}

/*
 * Demonstrate the semantic difference between locking and HTM:
 * Empty critical sections have time-based messaging semantics in
 * locking but are no-ops in HTM.
 */
static void demonstrate_semantic_difference(void)
{
    printf("\n=== Semantic Difference Demo ===\n");
    printf("With locking, an empty critical section guarantees that\n"
           "all previous holders have released the lock.\n"
           "With HTM/TLE, an empty transaction is a no-op.\n\n");

    pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
    volatile int ready = 0;

    /* Lock-based empty critical section - guarantees ordering */
    pthread_mutex_lock(&lock);
    ready = 1;
    pthread_mutex_unlock(&lock);

    /* Empty critical section waits for prior holder */
    pthread_mutex_lock(&lock);
    pthread_mutex_unlock(&lock);

    printf("After empty lock critical section, 'ready' is visible: %d\n", ready);

    /*
     * With HTM, the equivalent empty transaction would not provide
     * this time-based messaging guarantee.
     */
    printf("With HTM, an empty transaction would NOT provide this guarantee.\n");
}

/* Check if CPU supports RTM via CPUID */
static int check_tsx_support(void)
{
    unsigned int eax, ebx, ecx, edx;
    /* CPUID leaf 7, subleaf 0: check bit 11 of EBX (RTM) */
    __asm__ __volatile__("cpuid"
                         : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
                         : "a"(7), "c"(0));
    return (ebx >> 11) & 1;
}

int main(void)
{
    pthread_t threads[NR_THREADS];

    printf("TSX/HTM Demo\n");
    printf("Threads: %d, Iterations per thread: %d\n", NR_THREADS, ITERATIONS);

    if (!check_tsx_support()) {
        printf("\nTSX (RTM) not supported on this CPU.\n");
        printf("Modern Intel CPUs (12th Gen+) have TSX disabled by default\n");
        printf("due to security vulnerabilities (TAA, MDS).\n");
        printf("The kernel handles this in arch/x86/kernel/cpu/tsx.c\n");
        printf("This demo would fall back to locking entirely.\n");
        printf("Skipping TSX transaction test.\n\n");

        /* Still demonstrate the semantic difference */
        demonstrate_semantic_difference();
        return 0;
    }

    printf("\nNote: On modern Intel CPUs, TSX is disabled by default.\n");
    printf("      The kernel handles this in arch/x86/kernel/cpu/tsx.c\n");
    printf("      Fallback to locking will be used when TSX aborts.\n\n");

    for (int i = 0; i < NR_THREADS; i++)
        pthread_create(&threads[i], NULL, worker_thread, NULL);

    for (int i = 0; i < NR_THREADS; i++)
        pthread_join(threads[i], NULL);

    printf("Results:\n");
    printf("  Final counter:   %lu\n", (unsigned long)g_counter);
    printf("  Final metadata:  %lu\n", (unsigned long)g_metadata);
    printf("  TX commits:      %lu\n", (unsigned long)g_tx_commits);
    printf("  TX aborts:       %lu\n", (unsigned long)g_tx_aborts);
    printf("  Fallback locks:  %lu\n", (unsigned long)g_fallback_count);

    uint64_t expected = NR_THREADS * ITERATIONS;
    if (g_counter != expected) {
        printf("ERROR: counter mismatch (expected %lu)\n", (unsigned long)expected);
        return 1;
    }

    printf("Counter correct!\n");

    demonstrate_semantic_difference();

    return 0;
}
