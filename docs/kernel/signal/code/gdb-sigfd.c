/*
 * 验证 signalfd 的"截获"效果：
 * 当信号被 signalfd 监听且被 pthread_sigmask 阻塞后，
 * 传统的信号处理函数不会被调用，信号通过文件描述符传递。
 *
 * 这与 GDB 截获 SIGINT 有类似之处：
 * 信号不会走正常的异步 handler 路径，而是被"重定向"到另一处。
 *
 * 用法:
 *   ./signalfd-demo &
 *   kill -TERM <pid>
 *   # 观察输出：signal handler 不会执行，但 signalfd 会读到信号
 */
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <sys/signalfd.h>
#include <poll.h>
#include <stdlib.h>

#define pr_errno(msg) perror(msg)

int main(void)
{
	sigset_t mask;
	int ret;
	int sfd;

	/* 1. 阻塞 SIGINT，防止异步 delivery */
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	ret = pthread_sigmask(SIG_BLOCK, &mask, NULL);
	if (ret) {
		pr_errno("pthread_sigmask()");
		return -1;
	}
	printf("SIGINT blocked by pthread_sigmask.\n");

	/* 2. 创建 signalfd 来接收被阻塞的 SIGINT */
	sigemptyset(&mask);
	sigaddset(&mask, SIGINT);
	sfd = signalfd(-1, &mask, SFD_NONBLOCK | SFD_CLOEXEC);
	if (sfd == -1) {
		pr_errno("signalfd()");
		return -1;
	}
	printf("signalfd created (fd=%d), waiting for SIGINT...\n", sfd);
	printf("Run: kill -INT %d\n\n", getpid());

	/* 3. 用 poll 等待 signalfd 可读 */
	struct pollfd pfd = {
		.fd = sfd,
		.events = POLLIN,
	};

	while (1) {
		ret = poll(&pfd, 1, -1);
		if (ret == -1) {
			pr_errno("poll");
			break;
		}

		if (pfd.revents & POLLIN) {
			struct signalfd_siginfo fdsi;
			ssize_t s = read(sfd, &fdsi, sizeof(fdsi));
			if (s != sizeof(fdsi)) {
				pr_errno("read signalfd");
				break;
			}

			printf("[signalfd] Received signal %d (%s)\n",
			       fdsi.ssi_signo, strsignal(fdsi.ssi_signo));
			printf("[signalfd] ssi_pid=%d, ssi_uid=%d, ssi_code=%d\n",
			       fdsi.ssi_pid, fdsi.ssi_uid, fdsi.ssi_code);
			break;
		}
	}

	close(sfd);
	printf("\nProgram exit normally.\n");
	return 0;
}
