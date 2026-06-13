/*
 * 这里演示一个例子，对于 STOP 信号，
 * epoll_wait 是不会自动 restart 的。
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>

volatile sig_atomic_t got_signal = 0;
void handler(int sig)
{
	got_signal = 1;
	printf("Caught signal %d (%s)\n", sig, strsignal(sig));
}

int register_signal(void)
{
	struct sigaction sa;
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;
	if (sigaction(SIGWINCH, &sa, NULL) == -1) {
		perror("sigaction");
		return 1;
	}
	return 0;
}

int main(void)
{
	const int MAX_EVENTS = 10;
	struct epoll_event ev, events[MAX_EVENTS];
	int epfd = epoll_create1(0);
	if (epfd == -1) {
		perror("epoll_create1");
		exit(EXIT_FAILURE);
	}

	// 如果注册了 handler ，那么 epoll 会退出
	// 如果没有注册，window size change 的信号会被忽略
	// if (register_signal())
	// 	return 1;

	// 监听 stdin (fd 0)
	ev.events = EPOLLIN;
	ev.data.fd = STDIN_FILENO;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, STDIN_FILENO, &ev) == -1) {
		perror("epoll_ctl");
		close(epfd);
		exit(EXIT_FAILURE);
	}

	printf("等待输入（按 Ctrl+D 结束）...\n");
	printf("kill -STOP %d\n", getpid());
	printf("kill -CONT %d\n", getpid());

	while (1) {
		int nr = epoll_wait(epfd, events, MAX_EVENTS, -1);

		if (nr == -1) {
			// 如果 ctrl-z 然后 fg ，那么这里会检查到错误
			// epoll_wait: Interrupted system call
			perror("epoll_wait");
			break;
		}

		for (int i = 0; i < nr; i++) {
			if (events[i].data.fd == STDIN_FILENO) {
				char buf[1024];
				ssize_t n = read(STDIN_FILENO, buf,
						 sizeof(buf) - 1);
				if (n > 0) {
					buf[n] = '\0';
					printf("输入: %s", buf);
				} else if (n == 0) {
					printf("EOF，退出。\n");
					break;
				} else {
					perror("read");
					break;
				}
			}
		}
	}

	close(epfd);
	return 0;
}
