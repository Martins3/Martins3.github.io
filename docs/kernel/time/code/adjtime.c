#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <string.h>
#include <unistd.h>

/**
大家应该实际上在使用的是:

int adjtimex(struct timex *buf);
int clock_adjtime(clockid_t clk_id, struct timex *buf);
int do_sys_settimeofday64(const struct timespec64 *tv, const struct timezone
*tz)

1. int ntp_adjtime(struct timex *buf);  # 这个 linux 没有实现
https://man.freebsd.org/cgi/man.cgi?ntp_adjtim
2. do_sys_settimeofday64 是用于调整的

*/

static void adjtimex()
{
	struct timex delta;
	// 调整时钟
	if (clock_adjtime(CLOCK_REALTIME, &delta) == -1) {
		perror("clock_adjtime");
		exit(EXIT_FAILURE);
	}
	printf("Clock mode: %d\n", delta.modes);
	printf("Offset: %ld us\n", delta.offset);
	printf("Frequency: %ld\n", delta.freq);
	printf("Max error: %ld\n", delta.maxerror);
	printf("Est error: %ld\n", delta.esterror);
	printf("Status: %d\n", delta.status);
	printf("Time constant: %ld\n", delta.constant);
	printf("Precision: %ld us\n", delta.precision);
	printf("Tick: %ld us\n", delta.tick);
}
static int test_settimeofday()
{
	struct timeval tv;

	// 获取当前时间
	if (gettimeofday(&tv, NULL) == -1) {
		perror("gettimeofday");
		return 1;
	}

	printf("当前时间: %ld 秒 %ld 微秒\n", tv.tv_sec, tv.tv_usec);

	// 向前调整 10 秒
	tv.tv_sec += 1000;

	// 设置新时间
	if (settimeofday(&tv, NULL) == -1) {
		perror("settimeofday");
		return 1;
	}

	printf("已向前调整 10 秒\n");
	return 0;
}

int main(int argc, char *argv[])
{
	if (strcmp(argv[1], "1") == 0)
		adjtimex();

	if (strcmp(argv[1], "2") == 0)
		test_settimeofday();

	return 0;
}
