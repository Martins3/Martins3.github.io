// SPDX-License-Identifier: GPL-2.0
/*
 * 验证"绝对时间是在 time namespace 的视角里解释的"：
 * 程序接收一个绝对 CLOCK_MONOTONIC 时间（秒，可以是小数），用 timerfd 的
 * TIMER_ABSTIME 去等它，最多等 5s，然后报告是否过期。
 *
 *   gcc -O0 -o timens-abs timens-abs.c
 *   ./timens-abs 12345.2
 */
#define _GNU_SOURCE
#include <poll.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

static double real_now(void)
{
	struct timespec ts;

	clock_gettime(CLOCK_REALTIME, &ts);
	return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

int main(int argc, char **argv)
{
	struct itimerspec its = { 0 };
	struct pollfd pfd;
	struct timespec now;
	double abs_sec, now_sec, t0, t1;
	uint64_t exp;
	int fd, ret;

	if (argc < 2) {
		printf("usage: %s <absolute CLOCK_MONOTONIC seconds>\n",
		       argv[0]);
		return 1;
	}
	abs_sec = atof(argv[1]);

	clock_gettime(CLOCK_MONOTONIC, &now);
	now_sec = (double)now.tv_sec + (double)now.tv_nsec / 1e9;

	its.it_value.tv_sec = (time_t)abs_sec;
	its.it_value.tv_nsec = (long)((abs_sec - (double)(time_t)abs_sec) * 1e9);

	fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
	if (timerfd_settime(fd, TIMER_ABSTIME, &its, NULL) < 0) {
		perror("timerfd_settime");
		return 1;
	}

	printf("CLOCK_MONOTONIC now = %.6f, absolute = %.6f, delta = %+.3f s\n",
	       now_sec, abs_sec, abs_sec - now_sec);

	pfd.fd = fd;
	pfd.events = POLLIN;
	t0 = real_now();
	ret = poll(&pfd, 1, 5000);
	t1 = real_now();

	if (ret > 0) {
		read(fd, &exp, sizeof(exp));
		printf("  => EXPIRED after %.3f s (real time)\n", t1 - t0);
	} else {
		printf("  => not expired after %.3f s, poll timeout\n", t1 - t0);
	}

	return 0;
}
