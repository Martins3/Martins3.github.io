// https://man7.org/linux/man-pages/man2/pidfd_open.2.html

#define _GNU_SOURCE
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <unistd.h>

static int pidfd_open(pid_t pid, unsigned int flags)
{
	return syscall(SYS_pidfd_open, pid, flags);
}

int main(int argc, char *argv[])
{
	int pidfd, ready;
	struct pollfd pollfd;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <pid>\n", argv[0]);
		exit(EXIT_SUCCESS);
	}

	pidfd = pidfd_open(atoi(argv[1]), 0);
	if (pidfd == -1) {
		perror("pidfd_open");
		exit(EXIT_FAILURE);
	}

	pollfd.fd = pidfd;
	pollfd.events = POLLIN;

	ready = poll(&pollfd, 1, -1);
	if (ready == -1) {
		perror("poll");
		exit(EXIT_FAILURE);
	}

	// 不可以在这里进行 waitpid 了
	// TODO 但是到底如何才可以获取到观测 pid 的 exit reason 是一个问题
	printf("Events (%#x): POLLIN is %sset\n", pollfd.revents,
	       (pollfd.revents & POLLIN) ? "" : "not ");

	close(pidfd);
	exit(EXIT_SUCCESS);
}
