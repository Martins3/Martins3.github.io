#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>

static void die(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

static ssize_t xvmsplice(int fd, const struct iovec *iov, unsigned long nr_segs,
			 unsigned int flags)
{
	return syscall(SYS_vmsplice, fd, iov, nr_segs, flags);
}

int main(void)
{
	int pipefd[2];
	char part1[] = "vmsplice can attach user pages ";
	char part2[] = "to a pipe as pipe buffers\n";
	struct iovec iov[2] = {
		{ .iov_base = part1, .iov_len = strlen(part1) },
		{ .iov_base = part2, .iov_len = strlen(part2) },
	};
	char buf[128];
	ssize_t n;

	if (pipe(pipefd) < 0)
		die("pipe");

	n = xvmsplice(pipefd[1], iov, 2, 0);
	if (n < 0)
		die("vmsplice");
	printf("vmsplice wrote %zd bytes into the pipe\n", n);

	close(pipefd[1]);

	n = read(pipefd[0], buf, sizeof(buf) - 1);
	if (n < 0)
		die("read");
	buf[n] = '\0';

	printf("read back: %s", buf);

	close(pipefd[0]);

	return EXIT_SUCCESS;
}
