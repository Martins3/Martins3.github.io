/*
 * compiler_barrier.c: Demonstrate barrier() and READ_ONCE/WRITE_ONCE.
 *
 * Shows how compiler optimizations can break concurrent code,
 * and how to prevent them.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>

#define READ_ONCE(x) (*(volatile typeof(x) *)&(x))
#define WRITE_ONCE(x, val) (*(volatile typeof(x) *)&(x) = (val))
#define barrier() __asm__ __volatile__("" ::: "memory")

static int need_to_stop = 0;
static int status = 0;

#define SHUTTING_DOWN 1
#define SHUT_DOWN 2

/* Example 1: Without READ_ONCE, compiler might fuse loads */
static unsigned long loop_count_broken = 0;
static unsigned long loop_count_fixed = 0;

void *spin_wait_broken(void *arg)
{
	(void)arg;
	/* Without READ_ONCE, compiler may hoist the load out of loop.
	 * On high optimization, this could become an infinite loop.
	 * We add a safety bound to avoid hanging the demo.
	 */
	unsigned long safety = 0;
	while (!need_to_stop) {
		loop_count_broken++;
		if (++safety > 10000000)
			break;
	}
	return NULL;
}

void *spin_wait_fixed(void *arg)
{
	(void)arg;
	/* With READ_ONCE, compiler must reload need_to_stop each iteration */
	while (!READ_ONCE(need_to_stop)) {
		loop_count_fixed++;
	}
	return NULL;
}

/* Example 2: barrier() prevents reordering */
static int reorder_a = 0;
static int reorder_b = 0;

void *writer_reorder(void *arg)
{
	(void)arg;
	reorder_a = 1;
	barrier(); /* prevent compiler from reordering a and b */
	reorder_b = 1;
	return NULL;
}

void *reader_reorder(void *arg)
{
	(void)arg;
	while (!READ_ONCE(reorder_b))
		continue;
	barrier();
	printf("reader_reorder: reorder_a = %d (expected 1 if barrier works)\n",
	       READ_ONCE(reorder_a));
	return NULL;
}

/* Example 3: store fusing prevention with WRITE_ONCE */
void shut_it_down_fixed(void)
{
	WRITE_ONCE(status, SHUTTING_DOWN);
	/* ... do work ... */
	WRITE_ONCE(status, SHUT_DOWN);
}

int main(void)
{
	pthread_t tid;

	printf("=== Compiler Barrier and READ_ONCE/WRITE_ONCE Demo ===\n\n");

	/* Test 1: broken spin wait */
	printf("--- Broken spin wait (may run forever without READ_ONCE) ---\n");
	need_to_stop = 0;
	pthread_create(&tid, NULL, spin_wait_broken, NULL);
	usleep(1000); /* let thread start */
	need_to_stop = 1;
	pthread_join(tid, NULL);
	printf("loop_count_broken = %lu\n", loop_count_broken);

	/* Test 2: fixed spin wait */
	printf("\n--- Fixed spin wait with READ_ONCE ---\n");
	need_to_stop = 0;
	pthread_create(&tid, NULL, spin_wait_fixed, NULL);
	usleep(1000);
	need_to_stop = 1;
	pthread_join(tid, NULL);
	printf("loop_count_fixed = %lu\n", loop_count_fixed);
	printf("(Fixed version sees the update because READ_ONCE forces reload)\n");

	/* Test 3: reordering */
	printf("\n--- Reordering prevention with barrier() ---\n");
	reorder_a = 0;
	reorder_b = 0;
	pthread_t tid_reader, tid_writer;
	pthread_create(&tid_reader, NULL, reader_reorder, NULL);
	usleep(100);
	pthread_create(&tid_writer, NULL, writer_reorder, NULL);
	pthread_join(tid_writer, NULL);
	pthread_join(tid_reader, NULL);

	return 0;
}
