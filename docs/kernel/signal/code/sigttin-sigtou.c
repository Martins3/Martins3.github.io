/*
 * 演示 SIGTTIN 和 SIGTTOU 信号的产生与处理。
 *
 * 当后台进程组的进程尝试从控制终端读取时，内核向其发送 SIGTTIN；
 * 当后台进程组的进程尝试向控制终端写入（且终端 TOSTOP 标志置位）时，
 * 内核向其发送 SIGTTOU。
 * 默认行为都是停止（STOP）该进程。
 *
 * 用法:
 *   1. 前台运行: ./sigttin-sigtou
 *   2. 按 Ctrl+Z 将其挂起到后台
 *   3. 继续后台运行: bg
 *   4. 此时进程会收到 SIGTTIN（尝试 read）或 SIGTTOU（尝试 write）而被停止
 *   5. fg 将其切回前台即可继续
 *
 * 或者直接在终端里:
 *   ./sigttin-sigtou &   # 后台运行
 */
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>

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

	if (sigaction(SIGTTIN, &sa, NULL) == -1) {
		perror("sigaction SIGTTIN");
		return 1;
	}
	if (sigaction(SIGTTOU, &sa, NULL) == -1) {
		perror("sigaction SIGTTOU");
		return 1;
	}
	return 0;
}

int main(void)
{
	// if (register_signal())
	// 	return 1;

	printf("PID: %d\n", getpid());
	printf("PGID: %d\n", getpgrp());
	printf("TTY 前台进程组: %d\n", tcgetpgrp(STDIN_FILENO));

	printf("\n尝试从终端读取（将触发 SIGTTIN，如果在后台）...\n");

	char buf[1024];
	ssize_t n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
	if (n == -1) {
		perror("read");
	} else if (n == 0) {
		printf("收到 EOF\n");
	} else {
		buf[n] = '\0';
		printf("读到: %s", buf);
	}

	printf("\n尝试向终端写入（将触发 SIGTTOU，如果在后台且 TOSTOP 置位）...\n");
	printf("这行输出若在后台且 TOSTOP 生效，会产生 SIGTTOU\n");

	printf("\n程序正常结束。\n");
	return 0;
}
