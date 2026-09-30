/*
 * 将 1970 时间装换为日期
 */
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <stdint.h>
#include <stdlib.h>

static void ts_to_time_str(uint64_t ts_sec)
{
	char buf[100];
	time_t raw_time;
	struct tm *timeinfo;
	raw_time = ts_sec;
	timeinfo = localtime(&raw_time);
	strftime(buf, 100, "%Y-%m-%d %H:%M:%S", timeinfo);
	printf("%s\n", buf);
}

int main(int argc, char *argv[])
{
	uint64_t t;
	if (argc == 1) {
		printf("./timespec.out 1716358335");
		return 1;
	}
	sscanf(argv[1], "%ld", &t);
	ts_to_time_str(t);
	return 0;
}
