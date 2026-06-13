/*
 * 正如 man signal(7)
 *
 * 1. 体会下注册 sa.sa_flags = SA_RESTART 的差别
 * 2. 如果注册了 SA_RESTART ，那么无论 STOP 还是 USR1 ，
 * read(2) 这个 syscall 是可以自动恢复的，也就是就像是这个 syscall 总是在执行。
 *
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

volatile sig_atomic_t got_signal = 0;

void handler(int sig)
{
	got_signal = 1;
	printf("Caught signal %d (%s)\n", sig, strsignal(sig));
}

int main(void)
{
	struct sigaction sa;
	char buf[128];
	ssize_t n;

	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGUSR1, &sa, NULL) == -1) {
		perror("sigaction");
		return 1;
	}

	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;

	/*
	 * 需要区分 SIGSTOP 和 SIGTSTP :
	 * `SIGKILL` 和 `SIGSTOP` 是两个特殊的信号，既不能被捕获，也不能被阻塞或忽略。
	 *
	 * 这里我们注册了 SIGSTOP 之后，ctrl-z 就会被自动的捕获，而不是暂停程序。
	 */
	if (sigaction(SIGTSTP, &sa, NULL) == -1) {
		perror("sigaction");
		return 1;
	}

	printf("My PID is %d\n", getpid());
	printf("Run ' kill -USR1 %d ' in another terminal\n", getpid());
	printf("Run ' kill -STOP %d ' in another terminal\n", getpid());
	printf("Reading from stdin...\n");

	n = read(STDIN_FILENO, buf, sizeof(buf));

	if (n == -1) {
		perror("read");
		printf("errno = %d (%s)\n", errno, strerror(errno));
	} else {
		printf("Read %zd bytes\n", n);
	}

	printf("Got signal: %d\n", got_signal);

	return 0;
}
