/*
 * forkjoin.c: Demonstrate POSIX fork() and wait() primitives.
 * Based on perfbook CodeSamples/toolsoftrade/forkjoin.c
 *
 * This program shows that forked processes do NOT share memory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <errno.h>

int x = 0;

static void waitall(void)
{
	int pid;
	int status;

	for (;;) {
		pid = wait(&status);
		if (pid == -1) {
			if (errno == ECHILD)
				break;
			perror("wait");
			exit(EXIT_FAILURE);
		}
	}
}

int main(int argc, char *argv[])
{
	int nforks = 2;
	int pid;
	int i;

	if (argc > 1)
		nforks = atoi(argv[1]);

	printf("Parent starting, x = %d\n", x);

	for (i = 0; i < nforks; i++) {
		pid = fork();
		if (pid == 0) {
			/* child */
			x = 1;
			printf("Child %d set x = %d\n", getpid(), x);
			exit(0);
		} else if (pid < 0) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
	}

	/* parent waits for all children */
	waitall();

	/* Parent's x is unchanged because processes do not share memory */
	printf("Parent sees x = %d (expected 0, because fork() does not share memory)\n", x);

	return 0;
}
