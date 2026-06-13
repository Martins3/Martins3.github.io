/*
 * 测试 GDB 对 SIGINT 的处理行为。
 *
 * 问题:
 *   1. 如果注册了 SIGINT handler，在 GDB 中运行时被 Ctrl+C 打断，handler 会被调用吗?
 *   2. 如果没注册 SIGINT handler，为什么程序不会被 kill ?
 *
 * GDB 默认会将 SIGINT 捕获并停止程序（不传递给被调试进程），
 * 因此无论是否注册 handler，在 GDB 内按 Ctrl+C 时，
 * 首先都是 GDB 自己收到信号并暂停程序，而不是直接让程序的 handler 执行。
 *
 * 用法:
 *   gdb ./gdb-sigint
 *   (gdb) run
 *   # 按 Ctrl+C
 *   # 观察程序是否打印 "Caught signal 2"
 *   (gdb) info signals SIGINT   # 查看 GDB 对 SIGINT 的默认处置
 *   (gdb) continue              # 继续运行，默认不会把 SIGINT 传给程序
 *   (gdb) signal SIGINT         # 手动将 SIGINT 发给程序，此时 handler 才会执行
 */
#include <stdio.h>
#include <signal.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

void handler(int sig)
{
	printf("Caught signal %d (%s)\n", sig, strsignal(sig));
	exit(1);
}

int main(void)
{
	struct sigaction sa;
	sa.sa_handler = handler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = SA_RESTART;

	if (sigaction(SIGINT, &sa, NULL) == -1) {
		perror("sigaction");
		return 1;
	}

	printf("PID: %d\n", getpid());
	printf("SIGINT handler registered.\n");
	printf("在 GDB 中运行，按 Ctrl+C 观察信号处理行为...\n\n");

	while (1) {
		printf("sleeping\n");
		sleep(1);
	}

	return 0;
}
