// SPDX-License-Identifier: GPL-2.0
/*
 * 观察 unshare(CLONE_NEWTIME) 只影响 time_ns_for_children：
 *   - 调用者自己仍然在旧的 time namespace 里
 *   - fork 出来的孩子才进入新的 time namespace
 *   - 第一个孩子加入之后 offsets 被 freezed，再写就是 -EACCES
 *
 * 需要在 userns 里跑才有 CAP_SYS_ADMIN：
 *   unshare -Ur ./timens-fork
 *
 *   gcc -O0 -o timens-fork timens-fork.c
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static void show(const char *who)
{
	char buf[512] = { 0 };
	struct timespec mono, boot;
	int fd, n;

	clock_gettime(CLOCK_MONOTONIC, &mono);
	clock_gettime(CLOCK_BOOTTIME, &boot);
	printf("%-24s pid=%d MONOTONIC=%lld.%09ld BOOTTIME=%lld.%09ld\n", who,
	       getpid(), (long long)mono.tv_sec, mono.tv_nsec,
	       (long long)boot.tv_sec, boot.tv_nsec);

	fd = open("/proc/self/timens_offsets", O_RDONLY);
	if (fd < 0)
		return;
	n = read(fd, buf, sizeof(buf) - 1);
	close(fd);
	if (n > 0)
		printf("%-24s   timens_offsets:\n", who), fputs(buf, stdout);
}

static const char *g_offs = "monotonic 3600 0\nboottime 7200 0\n";

static int write_offsets(const char *what)
{
	const char *offs = g_offs;
	int fd = open("/proc/self/timens_offsets", O_WRONLY);
	ssize_t n;

	if (fd < 0) {
		printf("%-24s open: %s\n", what, strerror(errno));
		return -1;
	}
	n = write(fd, offs, strlen(offs));
	close(fd);
	if (n < 0) {
		printf("%-24s write: %s (errno=%d)\n", what, strerror(errno),
		       errno);
		return -1;
	}
	printf("%-24s write OK\n", what);
	return 0;
}

int main(int argc, char **argv)
{
	pid_t pid;
	int st;

	if (argc > 1)
		g_offs = argv[1];

	show("before unshare(CLONE_NEWTIME)");

	if (unshare(CLONE_NEWTIME) < 0) {
		printf("unshare(CLONE_NEWTIME): %s\n", strerror(errno));
		return 1;
	}

	show("after unshare");
	if (write_offsets("set offsets (parent)") < 0)
		return 1;
	show("after set offsets");

	fflush(stdout); /* fork 之后父子共享 stdio 缓冲区，先刷掉避免重复输出 */
	pid = fork();
	if (pid == 0) {
		show("child (in new timens)");
		write_offsets("set offsets (child)");
		exit(0);
	}

	waitpid(pid, &st, 0);

	show("parent");
	write_offsets("set offsets again");

	return 0;
}
