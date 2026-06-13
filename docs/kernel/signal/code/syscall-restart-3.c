/*
 * pause() 和 nanosleep() 一样，接受到除去 STOP，必然退出，
 */
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

	pause();

	printf("Got signal: %d\n", got_signal);

	return 0;
}
