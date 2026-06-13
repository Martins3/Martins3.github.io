/*
 * retrigger_gp.c - Retriggered grace periods state machine.
 * Corresponds to perfbook Section 13.5.6 "Retriggered Grace Periods".
 *
 * When open()/close() can race with RCU grace periods, a simple
 * call_rcu() is insufficient because open() might happen before
 * the callback runs.  This demo implements the state machine
 * CLOSED -> OPEN -> CLOSING -> [REOPENING/RECLOSING] -> ...
 */
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include "urcu_simple.h"

#define RTRG_CLOSED    0
#define RTRG_OPEN      1
#define RTRG_CLOSING   2
#define RTRG_REOPENING 3
#define RTRG_RECLOSING 4

static int rtrg_status = RTRG_CLOSED;
static pthread_mutex_t rtrg_lock = PTHREAD_MUTEX_INITIALIZER;
static int open_count = 0;
static int close_cb_count = 0;

/* Simulated call_rcu: we run callback after a short delay in another thread */
static struct rcu_head sim_rh;
static int pending_cb = 0;

static void close_cb(struct rcu_head *rhp)
{
	(void)rhp;
	pthread_mutex_lock(&rtrg_lock);
	close_cb_count++;
	if (rtrg_status == RTRG_CLOSING) {
		rtrg_status = RTRG_CLOSED;
		printf("  callback: CLOSING -> CLOSED\n");
	} else if (rtrg_status == RTRG_REOPENING) {
		rtrg_status = RTRG_OPEN;
		printf("  callback: REOPENING -> OPEN\n");
	} else if (rtrg_status == RTRG_RECLOSING) {
		rtrg_status = RTRG_CLOSING;
		printf("  callback: RECLOSING -> CLOSING (requeue GP)\n");
		pending_cb = 1;  /* simulate requeue */
	} else {
		printf("  callback: unexpected state %d\n", rtrg_status);
	}
	pthread_mutex_unlock(&rtrg_lock);
}

static void do_open(void)
{
	open_count++;
}

static void do_close(void)
{
}

static int my_open(void)
{
	pthread_mutex_lock(&rtrg_lock);
	if (rtrg_status == RTRG_CLOSED) {
		rtrg_status = RTRG_OPEN;
	} else if (rtrg_status == RTRG_CLOSING ||
		   rtrg_status == RTRG_RECLOSING) {
		rtrg_status = RTRG_REOPENING;
	} else {
		pthread_mutex_unlock(&rtrg_lock);
		return -1; /* busy */
	}
	do_open();
	pthread_mutex_unlock(&rtrg_lock);
	return 0;
}

static int my_close(void)
{
	pthread_mutex_lock(&rtrg_lock);
	if (rtrg_status == RTRG_OPEN) {
		rtrg_status = RTRG_CLOSING;
		pending_cb = 1;
	} else if (rtrg_status == RTRG_REOPENING) {
		rtrg_status = RTRG_RECLOSING;
	} else {
		pthread_mutex_unlock(&rtrg_lock);
		return -1;
	}
	do_close();
	pthread_mutex_unlock(&rtrg_lock);
	return 0;
}

/* Background thread to invoke callbacks after simulated grace period */
static void *cb_thread(void *arg)
{
	(void)arg;
	for (int i = 0; i < 20; i++) {
		{ struct timespec ts = {0, 1000000L}; nanosleep(&ts, NULL); }
		if (pending_cb) {
			pending_cb = 0;
			/* Simulate grace period elapsed */
			synchronize_rcu();
			close_cb(&sim_rh);
		}
	}
	return NULL;
}

int main(void)
{
	printf("[retrigger_gp] Retriggered grace period state machine\n");

	pthread_t cb;
	pthread_create(&cb, NULL, cb_thread, NULL);

	/* Scenario 1: normal open -> close */
	printf("Scenario 1: open -> close\n");
	my_open();
	printf("  state after open = %d (OPEN=1)\n", rtrg_status);
	my_close();
	printf("  state after close = %d (CLOSING=2)\n", rtrg_status);
	{ struct timespec ts = {0, 5000000L}; nanosleep(&ts, NULL); }  /* let callback run */

	/* Scenario 2: open -> close -> open (before GP ends) */
	printf("Scenario 2: open -> close -> open (reopen before GP)\n");
	my_open();
	my_close();
	printf("  state after close = %d\n", rtrg_status);
	my_open();  /* races with grace period */
	printf("  state after reopen = %d (REOPENING=3)\n", rtrg_status);
	{ struct timespec ts = {0, 5000000L}; nanosleep(&ts, NULL); }

	/* Scenario 3: open -> close -> open -> close */
	printf("Scenario 3: open -> close -> open -> close\n");
	my_close();
	printf("  state after second close = %d (RECLOSING=4)\n", rtrg_status);
	{ struct timespec ts = {0, 8000000L}; nanosleep(&ts, NULL); }  /* let requeued callback run */

	pthread_join(cb, NULL);

	printf("[OK] open_count=%d close_callbacks=%d final_state=%d\n",
	       open_count, close_cb_count, rtrg_status);
	return 0;
}
