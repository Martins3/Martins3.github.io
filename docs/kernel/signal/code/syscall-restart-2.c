/*
 * 正如 man signal(7) 中的描述，因为是 nanosleep
 * 1. 如果注册过 signal handler ，无论是否有 SA_RESTART ，必然失败
 * 2. 如果是 stop 信号，那么可以自动进入到 restart 中
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

volatile sig_atomic_t got_signal = 0;

void handler(int sig)
{
	got_signal = 1;
	printf("Caught signal %d (%s)\n", sig, strsignal(sig));
}

int main(void)
{
	struct sigaction sa;
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGUSR1, &sa, NULL) == -1) {
		perror("sigaction");
		return 1;
	}

	printf("My PID is %d\n", getpid());
	printf("Run ' kill -USR1 %d ' in another terminal\n", getpid());
	printf("Run ' kill -STOP %d ' in another terminal\n", getpid());
	printf("Reading from stdin...\n");

	struct timespec tim, tim2;
	tim.tv_sec = 10000;
	tim.tv_nsec = 0;

	if (nanosleep(&tim, &tim2) < 0) {
		printf("Nano sleep system call failed \n");
		printf("errno = %d (%s)\n", errno, strerror(errno));
		return -1;
	}

	printf("Got signal: %d\n", got_signal);

	return 0;
}
