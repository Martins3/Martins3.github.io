/*
 * function_shipping.c: Demonstrate function shipping using POSIX signals.
 *
 * When thread A needs to access data owned by thread B, instead of
 * reaching out directly (which may require locking), A "ships" a
 * function to B via a signal. B executes the function on its own data.
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

static __thread unsigned long thread_counter;
static unsigned long global_count;
static pthread_mutex_t gblcnt_mutex = PTHREAD_MUTEX_INITIALIZER;

static volatile int signal_received;
static volatile unsigned long signal_result;
static pthread_t worker_tid;
static volatile int worker_ready;

static void flush_sig_handler(int sig)
{
	(void)sig;
	/* This function runs in the context of the target thread.
	 * It operates on the target thread's own data (thread_counter).
	 */
	signal_result = thread_counter;
	signal_received = 1;
}

static void ship_function_to_thread(pthread_t tid)
{
	struct sigaction sa;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = flush_sig_handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	if (sigaction(SIGUSR1, &sa, NULL) != 0) {
		perror("sigaction");
		exit(EXIT_FAILURE);
	}

	signal_received = 0;
	pthread_kill(tid, SIGUSR1);

	while (!signal_received)
		usleep(100);

	pthread_mutex_lock(&gblcnt_mutex);
	global_count += signal_result;
	pthread_mutex_unlock(&gblcnt_mutex);
}

static void *worker_thread(void *arg)
{
	unsigned long i;
	(void)arg;

	for (i = 0; i < 5000000; i++)
		thread_counter++;

	worker_ready = 1;

	/* Wait for signal collection before exiting */
	while (worker_ready)
		usleep(100);

	return NULL;
}

int main(void)
{
	int rc;
	void *res;

	worker_ready = 0;

	rc = pthread_create(&worker_tid, NULL, worker_thread, NULL);
	if (rc != 0) {
		perror("pthread_create");
		exit(EXIT_FAILURE);
	}

	/* Wait for worker to finish counting */
	while (!worker_ready)
		usleep(100);

	/* Ship function to the worker thread to collect its local count */
	ship_function_to_thread(worker_tid);

	/* Allow worker to exit */
	worker_ready = 0;

	rc = pthread_join(worker_tid, &res);
	if (rc != 0) {
		perror("pthread_join");
		exit(EXIT_FAILURE);
	}

	printf("global_count after collection: %lu\n", global_count);
	printf("function shipping demo completed\n");
	return 0;
}
