/*
 * dynticks_like.c
 *
 * Simulates the dynticks_progress_counter mechanism used in early Linux
 * kernel preemptible RCU (around v2.6.25-rc4), as discussed in perfbook
 * Chapter 12 Section 12.1.4 (dynticks and Preemptible RCU).
 *
 * The core idea: a per-CPU counter is even when the CPU is in dynticks-idle
 * mode (no tasks or interrupts running), and odd otherwise. RCU grace-period
 * machinery uses snapshots of this counter to determine when a CPU may safely
 * be ignored because it cannot be executing RCU read-side critical sections.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <unistd.h>

/* Simulated per-CPU state */
static atomic_long dynticks_progress_counter = 0; /* even = idle, odd = active */
static atomic_long rcu_dyntick_snapshot = 0;

/* Enter dynticks-idle mode (e.g., CPU goes idle) */
static void rcu_enter_nohz(void)
{
	/* memory barrier omitted in simulation */
	atomic_fetch_add(&dynticks_progress_counter, 1);
	long val = atomic_load(&dynticks_progress_counter);
	if (val & 0x1) {
		printf("WARN: dynticks counter odd after enter_nohz!\n");
	}
}

/* Exit dynticks-idle mode (e.g., CPU starts running a task) */
static void rcu_exit_nohz(void)
{
	atomic_fetch_add(&dynticks_progress_counter, 1);
	long val = atomic_load(&dynticks_progress_counter);
	if (!(val & 0x1)) {
		printf("WARN: dynticks counter even after exit_nohz!\n");
	}
	/* memory barrier omitted in simulation */
}

/* Grace-period machinery: save snapshot */
static void dyntick_save_progress_counter(void)
{
	atomic_store(&rcu_dyntick_snapshot,
	             atomic_load(&dynticks_progress_counter));
}

/* Check if CPU remained in dynticks-idle since snapshot */
static int rcu_try_flip_waitack_needed(void)
{
	long curr = atomic_load(&dynticks_progress_counter);
	long snap = atomic_load(&rcu_dyntick_snapshot);

	/* If remained idle entire time */
	if ((curr == snap) && ((curr & 0x1) == 0))
		return 0;
	/* If passed through or entered idle */
	if ((curr - snap) >= 2 || (curr & 0x1) == 0)
		return 0;
	return 1;
}

/* Simulate a CPU that repeatedly enters/exits idle */
static void *cpu_sim(void *arg)
{
	(void)arg;
	for (int i = 0; i < 1000; i++) {
		rcu_exit_nohz();   /* start running */
		rcu_enter_nohz();  /* go idle */
	}
	return NULL;
}

/* Grace-period thread that takes snapshots and checks */
static void *gp_sim(void *arg)
{
	(void)arg;
	for (int i = 0; i < 500; i++) {
		dyntick_save_progress_counter();
		usleep(10);
		int needed = rcu_try_flip_waitack_needed();
		(void)needed;
	}
	return NULL;
}

int main(void)
{
	pthread_t cpu, gp;

	/* Start with even (idle) counter */
	atomic_store(&dynticks_progress_counter, 0);

	pthread_create(&cpu, NULL, cpu_sim, NULL);
	pthread_create(&gp, NULL, gp_sim, NULL);

	pthread_join(cpu, NULL);
	pthread_join(gp, NULL);

	long final = atomic_load(&dynticks_progress_counter);
	printf("Final dynticks counter: %ld (%s)\n",
	       final, (final & 0x1) ? "odd (active)" : "even (idle)");
	printf("Dynticks simulation completed successfully.\n");
	return 0;
}
