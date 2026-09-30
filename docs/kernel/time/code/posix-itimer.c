#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

void timer_handler(int signo)
{
	printf("Timer expired!\n");
}

int main(int argc, char *argv[])
{
	struct itimerval itv;

	// 设置信号处理函数
	signal(SIGALRM, timer_handler);

	// 让时间为 5s 开始，然后之后每秒触发一次
	itv.it_interval.tv_sec = 1;
	itv.it_interval.tv_usec = 0;
	itv.it_value.tv_sec = 5;
	itv.it_value.tv_usec = 0;
	if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
		perror("setitimer");
		exit(EXIT_FAILURE);
	}

	// 获取间隔定时器时间
	if (getitimer(ITIMER_VIRTUAL, &itv) == -1) {
		perror("getitimer");
		exit(EXIT_FAILURE);
	}
	// TODO 为什么打印的就结果少了一点点
	// Interval timer time: 4 seconds, 999998 microseconds
	printf("Interval timer time: %ld seconds, %ld microseconds\n",
	       itv.it_value.tv_sec, itv.it_value.tv_usec);

	for (size_t i = 0; i < 30; i++) {
		sleep(1);
		printf(".\n");
	}

	return 0;
}
