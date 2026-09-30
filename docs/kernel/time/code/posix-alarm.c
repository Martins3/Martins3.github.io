/*
 * man alarm(2)
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

void alarm_handler(int signo)
{
	printf("Alarm expired!\n");
}

int main()
{
	// 设置 alarm
	signal(SIGALRM, alarm_handler);
	alarm(3);
	for (size_t i = 0; i < 10; i++) {
		sleep(1);
		printf(".\n");
	}

	return 0;
}
