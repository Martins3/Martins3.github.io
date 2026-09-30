#define _XOPEN_SOURCE 600
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

static long displayClock()
{
	clockid_t clock = CLOCK_MONOTONIC_RAW;
	struct timespec ts;

	if (clock_gettime(clock, &ts) == -1) {
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}

	printf("%jd\n", (intmax_t)ts.tv_sec);
	return ts.tv_sec;
}

static bool diff_in_range(long a, long b)
{
	if (a - b > 0)
		return a - b <= 1;
	else
		return b - a <= 1;
}

#define CHECK_PERIOD 5

/* 用于检查时间发生跳变的场景
 *
 * 为什么可以在虚拟机中检测出来这个问题:
 *
 * 时间 jump foreward 的时候 ，sleep 会提前醒过来，但是获取 raw clock 的时间会继续保持
 *
 * 猜测 hrtimer 的时间中是没有办法做重新排列的，但是
 *
 * 而 sleep(1) 恰好确定时间。
 */
int main(int argc, char *argv[])
{
	long last_sec = 0;
	last_sec = displayClock();
	sleep(CHECK_PERIOD);

	while (true) {
		long sec = displayClock();
		if (!diff_in_range(sec, last_sec + CHECK_PERIOD)) {
			/* 虚拟机暂停和热迁移的时候也会检查出来异常
			 */
			printf("expected %jd, get %jd\n",
			       last_sec + CHECK_PERIOD, sec);
			exit(EXIT_FAILURE);
		}
		last_sec = sec;
		sleep(CHECK_PERIOD);
	}

	exit(EXIT_SUCCESS);
}
