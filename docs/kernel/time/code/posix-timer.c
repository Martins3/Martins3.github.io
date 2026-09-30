#define _GNU_SOURCE // 为了获取到 clock_adjtime
/*
 * When this option is disabled, the following syscalls won't be available:
 * 	  timer_create,
 * 	  timer_gettime
 * 	  timer_getoverrun
 * 	  timer_settime
 * 	  timer_delete
 *
 * 	  getitimer,
 * 	  setitimer
 *
 * 	  clock_adjtime
 * 	  alarm
 *
 * Furthermore, the clock_settime, clock_gettime,
 * clock_getres and clock_nanosleep syscalls will be limited to
 * CLOCK_REALTIME, CLOCK_MONOTONIC and CLOCK_BOOTTIME only.
 *
 * 	  TODO 测试 clock_settime 的效果
 *
 * 	  clock_nanosleep(CLOCK_REALTIME, 0, {tv_sec=10000, tv_nsec=0}
*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

static void timer_handler(int signo)
{
	printf("Timer expired!\n");
}

/*
 * 因为当前的 timer 是通过信号来触发的，在屏蔽信号的时候，如果
 * 当时有新的 timer 被 trigger 了。
 */
static void getoverrun(timer_t timerid)
{
	// 获取定时器超限次数
	int overrun = timer_getoverrun(timerid);
	if (overrun == -1) {
		perror("timer_getoverrun");
		exit(EXIT_FAILURE);
	}
	printf("Timer overrun count: %d\n", overrun);
}

int main()
{
	timer_t timerid;
	struct sigevent sev;
	struct itimerspec its;

	// 创建定时器
	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGALRM;
	sev.sigev_value.sival_ptr = &timerid;
	if (timer_create(CLOCK_REALTIME, &sev, &timerid) == -1) {
		perror("timer_create");
		exit(EXIT_FAILURE);
	}

	// 设置定时器
	its.it_value.tv_sec = 5;
	its.it_value.tv_nsec = 0;
	its.it_interval.tv_sec = 1;
	its.it_interval.tv_nsec = 0;
	if (timer_settime(timerid, 0, &its, NULL) == -1) {
		perror("timer_settime");
		exit(EXIT_FAILURE);
	}

	// 获取定时器时间
	if (timer_gettime(timerid, &its) == -1) {
		perror("timer_gettime");
		exit(EXIT_FAILURE);
	}
	printf("Timer time: %ld seconds, %ld nanoseconds\n",
	       its.it_value.tv_sec, its.it_value.tv_nsec);

	// 设置信号处理函数
	signal(SIGALRM, timer_handler);

	for (size_t i = 0; i < 30; i++) {
		sleep(1);
		printf(".\n");
		getoverrun(timerid);
	}

	// 删除定时器
	if (timer_delete(timerid) == -1) {
		perror("timer_delete");
		exit(EXIT_FAILURE);
	}

	return 0;
}
