// SPDX-License-Identifier: GPL-2.0
/*
 * 造出一个"还没有人 join"的 time namespace，方便从外部观察
 * /proc/PID/timens_offsets 可写窗口：
 *
 *   t=0   unshare(CLONE_NEWTIME) + 写 offsets，此时 frozen_offsets=false
 *   t=4   fork 一个孩子（孩子 join 这个 timens，frozen_offsets 变 true）
 *
 *   unshare -Ur ./timens-hold
 *
 *   gcc -O0 -o timens-hold timens-hold.c
 */
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void set_offsets(void)
{
	const char *offs = "monotonic 1000 0\n";
	int fd = open("/proc/self/timens_offsets", O_WRONLY);

	if (fd < 0) {
		printf("open: %s\n", strerror(errno));
		return;
	}
	if (write(fd, offs, strlen(offs)) < 0)
		printf("write: %s\n", strerror(errno));
	else
		printf("offsets written\n");
	close(fd);
}

int main(void)
{
	pid_t pid;

	if (unshare(CLONE_NEWTIME) < 0) {
		printf("unshare(CLONE_NEWTIME): %s\n", strerror(errno));
		return 1;
	}
	set_offsets();
	printf("pid=%d: nobody joined yet, frozen_offsets=false, sleep 4s\n",
	       getpid());
	fflush(stdout);
	sleep(4);

	pid = fork();
	if (pid == 0) {
		sleep(6);
		exit(0);
	}
	printf("pid=%d: child %d joined -> frozen, sleep 6s\n", getpid(), pid);
	fflush(stdout);
	sleep(6);

	return 0;
}
