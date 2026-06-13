/*
 * priority_boost_demo.c
 *
 * Demonstrate priority boosting semantics and how transactional lock
 * elision (TLE) can break them, as discussed in perfbook Section 17.3.5.5.
 *
 * In lock-based systems, empty critical sections can be used with
 * priority boosting to ensure a low-priority thread runs periodically.
 * With HTM/TLE, the empty critical section becomes an empty transaction
 * (a no-op), breaking this mechanism.
 *
 * This is a simplified demonstration of the concept from Listing 17.3
 * in the book.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdint.h>
#include <stdatomic.h>

/* Two locks used for priority boosting */
static pthread_mutex_t boost_lock[2];
static atomic_int g_boostee_ran = 0;
static atomic_int g_stop = 0;

/*
 * boostee - low priority thread that must run periodically.
 * It always holds one of the two locks.
 */
static void *boostee(void *arg)
{
    (void)arg;
    int i = 0;

    /* Acquire first lock before system becomes busy */
    pthread_mutex_lock(&boost_lock[i]);

    while (!atomic_load(&g_stop)) {
        pthread_mutex_lock(&boost_lock[!i]);
        pthread_mutex_unlock(&boost_lock[i]);
        i = i ^ 1;

        atomic_fetch_add(&g_boostee_ran, 1);

        /* Simulate doing some work */
        usleep(100);
    }

    pthread_mutex_unlock(&boost_lock[i]);
    return NULL;
}

/*
 * booster - high priority thread that uses empty critical sections
 * to boost the priority of boostee().
 */
static void *booster(void *arg)
{
    (void)arg;
    int i = 0;

    while (!atomic_load(&g_stop)) {
        usleep(500); /* sleep 0.5 ms */

        /*
         * Empty critical section: with locking, this guarantees
         * that boostee (if holding this lock) gets to run.
         * With HTM/TLE, this becomes a no-op transaction!
         */
        pthread_mutex_lock(&boost_lock[i]);
        pthread_mutex_unlock(&boost_lock[i]);
        i = i ^ 1;
    }

    return NULL;
}

int main(void)
{
    pthread_t t_boostee, t_booster;

    pthread_mutex_init(&boost_lock[0], NULL);
    pthread_mutex_init(&boost_lock[1], NULL);

    printf("Priority Boosting Demo\n");
    printf("========================\n");
    printf("boostee() always holds one lock and runs periodically.\n");
    printf("booster() uses empty critical sections to ensure boostee runs.\n");
    printf("\nWith pure locking, this works correctly.\n");
    printf("With HTM/TLE, empty critical sections become no-ops,\n");
    printf("breaking the priority-boosting mechanism.\n\n");

    pthread_create(&t_boostee, NULL, boostee, NULL);
    pthread_create(&t_booster, NULL, booster, NULL);

    /* Let them run for a short time */
    sleep(1);
    atomic_store(&g_stop, 1);

    pthread_join(t_boostee, NULL);
    pthread_join(t_booster, NULL);

    int ran = atomic_load(&g_boostee_ran);
    printf("boostee ran %d times in 1 second\n", ran);

    if (ran > 100) {
        printf("PASS: boostee ran frequently (locking works)\n");
    } else {
        printf("FAIL: boostee did not run enough\n");
    }

    printf("\nKey insight: If HTM elides the empty critical sections in\n");
    printf("booster(), the priority boosting mechanism breaks completely.\n");

    pthread_mutex_destroy(&boost_lock[0]);
    pthread_mutex_destroy(&boost_lock[1]);
    return 0;
}
