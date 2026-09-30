#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>

// 取反，然后向左移动三位，与上 CLOCKFD
#define CLOCKFD 3
#define FD_TO_CLOCKID(fd) ((~(clockid_t)(fd) << 3) | CLOCKFD)
#define CLOCKID_TO_FD(clk) ((unsigned int)~((clk) >> 3))
#define SECS_IN_DAY (24 * 60 * 60)

/*
 * 可以获取输出为，需要使用 sudo 来执行
 * dynamic clocks : 1739283690.050 (20130 days + 14h 21m 30s)
 */

int main(int argc, char *argv[])
{
	struct timespec ts;
	clockid_t clkid;
	int fd;

	fd = open("/dev/ptp0", O_RDWR);
	if (fd < 0) {
		perror("/dev/ptp0");
		return 1;
	}
	clkid = FD_TO_CLOCKID(fd);
	clock_gettime(clkid, &ts);
	printf("[martins3:%s:%d] %x\n", __FUNCTION__, __LINE__, clkid);
	printf("[martins3:%s:%d] %x\n", __FUNCTION__, __LINE__, clkid >> 3);
	printf("[martins3:%s:%d] %x\n", __FUNCTION__, __LINE__, ~(clkid >> 3));

	printf("%-15s: %10jd.%03ld (", "dynamic clocks", (intmax_t)ts.tv_sec,
	       ts.tv_nsec / 1000000);

	long days = ts.tv_sec / SECS_IN_DAY;
	if (days > 0)
		printf("%ld days + ", days);

	printf("%2dh %2dm %2ds", (int)(ts.tv_sec % SECS_IN_DAY) / 3600,
	       (int)(ts.tv_sec % 3600) / 60, (int)ts.tv_sec % 60);
	printf(")\n");

	return EXIT_SUCCESS;
}
