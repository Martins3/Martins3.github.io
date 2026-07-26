#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/types.h>
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
	char target[PATH_MAX];
	ssize_t n;

	snprintf(link_path, sizeof(link_path), "/proc/self/fd/%d", fd);
	n = readlink(link_path, target, sizeof(target) - 1);
	if (n < 0)
		die("readlink");

	target[n] = '\0';
	printf("%s fd=%d -> %s\n", name, fd, target);
	fflush(stdout);
}

static void show_unnamed_pipe(void)
{
	int pipefd[2];

	if (pipe2(pipefd, O_CLOEXEC) < 0)
		die("pipe2");

	puts("unnamed pipe: pipe2() returns two connected fds directly");
	print_fd_target("  read end ", pipefd[0]);
	print_fd_target("  write end", pipefd[1]);

	close(pipefd[0]);
	close(pipefd[1]);
}

static void create_fifo_with_raw_mknodat(const char *path)
{
	long ret;

	ret = syscall(SYS_mknodat, AT_FDCWD, path, S_IFIFO | 0600, 0);
	if (ret < 0)
		die("mknodat S_IFIFO");

	printf("raw syscall: mknodat(AT_FDCWD, \"%s\", S_IFIFO|0600, 0)\n",
	       path);
}

static void show_named_fifo(const char *fifo_path)
{
	const char msg[] = "hello through a named FIFO\n";
	struct stat st;
	pid_t pid;

	if (mkfifo(fifo_path, 0600) < 0)
		die("mkfifo");

	if (stat(fifo_path, &st) < 0)
		die("stat fifo");

	printf("named pipe: mkfifo() created %s\n", fifo_path);
	printf("  stat says S_ISFIFO(mode) = %s\n",
	       S_ISFIFO(st.st_mode) ? "true" : "false");
	fflush(stdout);

	pid = fork();
	if (pid < 0)
		die("fork");

	if (pid == 0) {
		char buf[128];
		ssize_t n;
		int fd;

		puts("reader: open(O_RDONLY) waits until a writer opens the FIFO");
		fflush(stdout);

		fd = open(fifo_path, O_RDONLY);
		if (fd < 0)
			die("reader open");

		print_fd_target("reader opened", fd);

		n = read(fd, buf, sizeof(buf) - 1);
		if (n < 0)
			die("reader read");

		buf[n] = '\0';
		printf("reader got %zd bytes: %s", n, buf);

		close(fd);
		exit(EXIT_SUCCESS);
	}

	sleep(1);

	int fd = open(fifo_path, O_WRONLY);
	if (fd < 0)
		die("writer open");

	print_fd_target("writer opened", fd);

	if (write(fd, msg, strlen(msg)) != (ssize_t)strlen(msg))
		die("writer write");

	close(fd);

	if (waitpid(pid, NULL, 0) < 0)
		die("waitpid");
}

int main(void)
{
	char dir_template[] = "/tmp/named-fifo-demo.XXXXXX";
	char fifo_path[PATH_MAX];
	char raw_fifo_path[PATH_MAX];
	char *dir;

	show_unnamed_pipe();
	puts("");

	dir = mkdtemp(dir_template);
	if (dir == NULL)
		die("mkdtemp");

	if (snprintf(fifo_path, sizeof(fifo_path), "%s/fifo-by-mkfifo", dir) >=
	    (int)sizeof(fifo_path))
		die("snprintf fifo_path");
	if (snprintf(raw_fifo_path, sizeof(raw_fifo_path), "%s/fifo-by-mknodat",
		     dir) >= (int)sizeof(raw_fifo_path))
		die("snprintf raw_fifo_path");

	show_named_fifo(fifo_path);

	puts("");
	create_fifo_with_raw_mknodat(raw_fifo_path);
	puts("mkfifo(3) is a libc interface; the kernel object is an S_IFIFO node.");

	unlink(fifo_path);
	unlink(raw_fifo_path);
	rmdir(dir);

	return EXIT_SUCCESS;
}
