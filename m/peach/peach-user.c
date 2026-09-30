// SPDX-License-Identifier: GPL-2.0
#define USERSPACE
#include "peach.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

int main(void)
{
	int fd, i, attempt, retries = 0;

	fd = open("/dev/peach", O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		perror("open /dev/peach");
		return 1;
	}
	if (ioctl(fd, PEACH_PROBE) < 0) {
		perror("PEACH_PROBE");
		close(fd);
		return 1;
	}
	/* IRQ 会让本次运行返回 EAGAIN，交还 Linux 处理后重新运行。 */
	for (i = 0; i < 100; i++) {
		for (attempt = 0; attempt < 100; attempt++) {
			if (ioctl(fd, PEACH_RUN) == 0)
				break;
			if (errno != EAGAIN) {
				perror("PEACH_RUN");
				close(fd);
				return 1;
			}
			retries++;
			usleep(1000);
		}
		if (attempt == 100) {
			fputs("PEACH_RUN: too many interrupted attempts\n",
			      stderr);
			close(fd);
			return 1;
		}
	}
	/* 未定义的命令必须拒绝，不能悄悄返回成功。 */
	if (ioctl(fd, _IO(PEACH_MAGIC, 127)) != -1 || errno != ENOTTY) {
		fputs("unknown ioctl did not return ENOTTY\n", stderr);
		close(fd);
		return 1;
	}
	close(fd);
	printf("PASS: 100 CPUID -> HLT runs, %d interrupted retries\n",
	       retries);
	return 0;
}
