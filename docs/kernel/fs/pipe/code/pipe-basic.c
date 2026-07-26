#define _GNU_SOURCE
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

static void die(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

static void print_fd_target(const char *name, int fd)
{
	char link_path[64];
	char target[256];
	ssize_t n;

	snprintf(link_path, sizeof(link_path), "/proc/self/fd/%d", fd);
	n = readlink(link_path, target, sizeof(target) - 1);
	if (n < 0)
		die("readlink");

	target[n] = '\0';
	printf("%s fd=%d -> %s\n", name, fd, target);
}

int main(void)
{
	int pipefd[2];
	const char msg[] = "message from parent through an anonymous pipe\n";
	pid_t pid;

	if (pipe2(pipefd, O_CLOEXEC) < 0)
		die("pipe2");

	print_fd_target("read end ", pipefd[0]);
	print_fd_target("write end", pipefd[1]);
	fflush(stdout);

	pid = fork();
	if (pid < 0)
		die("fork");

	if (pid == 0) {
		char buf[128];
		ssize_t n;

		close(pipefd[1]);

		n = read(pipefd[0], buf, sizeof(buf) - 1);
		if (n < 0)
			die("child read");
		buf[n] = '\0';
		printf("child read %zd bytes: %s", n, buf);

		n = read(pipefd[0], buf, sizeof(buf));
		if (n < 0)
			die("child read eof");
		printf("child second read returns %zd, because writer is closed\n",
		       n);

		close(pipefd[0]);
		exit(EXIT_SUCCESS);
	}

	close(pipefd[0]);

	if (write(pipefd[1], msg, strlen(msg)) != (ssize_t)strlen(msg))
		die("parent write");

	int queued = -1;
	if (ioctl(pipefd[1], FIONREAD, &queued) == 0)
		printf("parent sees queued bytes with FIONREAD: %d\n", queued);

	close(pipefd[1]);

	if (waitpid(pid, NULL, 0) < 0)
		die("waitpid");

	return EXIT_SUCCESS;
}
