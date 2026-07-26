#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* 每次拷贝的数据量：10 GiB */
static const size_t SIZE = 10ULL * 1024 * 1024 * 1024;

/* 是否使用 O_DIRECT 打开输入/输出文件；改成 0 即可对比普通缓冲 IO */
static const int USE_ODIRECT = 1;

static volatile sig_atomic_t stop;

static void handle_signal(int sig)
{
	(void)sig;
	stop = 1;
}

static void die(const char *msg)
{
	perror(msg);
	exit(EXIT_FAILURE);
}

static int create_temp_fd(char *template, const char *name)
{
	int flags = USE_ODIRECT ? O_DIRECT : 0;
	int fd = mkostemp(template, flags);

	if (fd < 0 && USE_ODIRECT) {
		fprintf(stderr,
			"%s: O_DIRECT open failed (%s), falling back to buffered\n",
			name, strerror(errno));
		fd = mkstemp(template);
	}

	if (fd < 0)
		die(USE_ODIRECT ? "mkostemp/mkstemp" : "mkstemp");

	return fd;
}

static void splice_all(int in_fd, int out_fd, size_t len)
{
	int pipefd[2];

	if (pipe(pipefd) < 0)
		die("pipe");

	while (len > 0) {
		ssize_t moved;

		moved = splice(in_fd, NULL, pipefd[1], NULL, len,
			       SPLICE_F_MORE);
		if (moved < 0) {
			if (errno == EINTR)
				continue;
			die("splice file to pipe");
		}
		if (moved == 0)
			break;

		len -= (size_t)moved;
		while (moved > 0) {
			ssize_t out = splice(pipefd[0], NULL, out_fd, NULL,
					     (size_t)moved, SPLICE_F_MORE);

			if (out < 0) {
				if (errno == EINTR)
					continue;
				die("splice pipe to file");
			}
			if (out == 0)
				die("short splice pipe to file");

			moved -= out;
		}
	}

	close(pipefd[0]);
	close(pipefd[1]);
}

int main(void)
{
	char in_template[] = "/home/martins3/splice-copy-input.XXXXXX";
	char out_template[] = "/home/martins3/splice-copy-output.XXXXXX";
	int in_fd;
	int out_fd;
	unsigned long long total = 0;
	unsigned long loops = 0;

	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	in_fd = create_temp_fd(in_template, "input");
	out_fd = create_temp_fd(out_template, "output");

	/* 预先分配 10 GiB 的输入文件 */
	if (fallocate(in_fd, 0, 0, SIZE) < 0)
		die("fallocate input");

	/* 输出文件大小固定为 10 GiB */
	if (ftruncate(out_fd, (off_t)SIZE) < 0)
		die("ftruncate output");

	printf("input : %s\n", in_template);
	printf("output: %s\n", out_template);
	printf("O_DIRECT: %s\n", USE_ODIRECT ? "yes" : "no");
	printf("copying %.2f GiB repeatedly via file -> pipe -> file splice\n",
	       (double)SIZE / (1024.0 * 1024.0 * 1024.0));
	fflush(stdout);

	while (!stop) {
		splice_all(in_fd, out_fd, SIZE);

		if (lseek(in_fd, 0, SEEK_SET) < 0)
			die("lseek input");
		if (lseek(out_fd, 0, SEEK_SET) < 0)
			die("lseek output");

		total += SIZE;
		loops++;
	}

	printf("loops: %lu, total copied: %.2f GiB\n",
	       loops, (double)total / (1024.0 * 1024.0 * 1024.0));

	unlink(in_template);
	unlink(out_template);

	close(in_fd);
	close(out_fd);

	return EXIT_SUCCESS;
}
