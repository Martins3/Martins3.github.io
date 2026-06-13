#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
	pid_t pid = fork();

	if (pid == -1) {
		perror("fork");
		return 1;
	} else if (pid == 0) {
		// Child process
		printf("Child process (PID: %d) is sleeping for 2 seconds\n",
		       getpid());
		sleep(2);
		printf("Child process (PID: %d) exiting\n", getpid());
	} else {
		// Parent process
		int status;
		pid_t wait_pid = waitpid(pid, &status, 0);

		if (wait_pid == -1) {
			perror("waitpid");
			return 1;
		}

		if (WIFEXITED(status)) {
			printf("Child process (PID: %d) exited with status: %d\n",
			       wait_pid, WEXITSTATUS(status));
		} else {
			printf("Child process (PID: %d) terminated abnormally\n",
			       wait_pid);
		}
	}

	return 0;
}
