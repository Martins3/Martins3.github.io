/*
 * process_ownership.c: Demonstrate data ownership via multiple processes.
 *
 * Each process has its own private address space, so all data is owned
 * by that process. This eliminates synchronization overhead entirely.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#define ITERATIONS 10000000

static unsigned long counter = 0;

static void compute_it(int id)
{
	unsigned long i;
	for (i = 0; i < ITERATIONS; i++)
		counter++;
	printf("process %d: counter = %lu\n", id, counter);
}

int main(void)
{
	pid_t pid1, pid2;
	int status;

	pid1 = fork();
	if (pid1 < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	if (pid1 == 0) {
		compute_it(1);
		_exit(EXIT_SUCCESS);
	}

	pid2 = fork();
	if (pid2 < 0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	if (pid2 == 0) {
		compute_it(2);
		_exit(EXIT_SUCCESS);
	}

	waitpid(pid1, &status, 0);
	waitpid(pid2, &status, 0);

	printf("parent: both children completed, no shared data needed sync\n");
	return 0;
}
