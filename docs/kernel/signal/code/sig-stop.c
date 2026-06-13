/*
 * 测试发送 signal stop 效果
 *
 * 先执行 sleep 1000 并且获取 pid
 * 然后用这个程序发送信号，被 stop 的 sleep 可以被 fg 唤醒
 *
 * ctrl-z 并不会使用 kill 系统调用来
 * 在 bash 中执行 ctrl-z ，使用 killsnoop 来观察，并没有什么东西
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <target_pid>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	pid_t pid = atoi(argv[1]);
	if (pid <= 0) {
		fprintf(stderr, "Invalid PID: %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	// 发送 SIGSTOP 信号
	if (kill(pid, SIGSTOP) == -1) {
		perror("kill(SIGSTOP) failed");
		exit(EXIT_FAILURE);
	}

	printf("Sent SIGSTOP to PID %d. It should be stopped now.\n", (int)pid);

	char cmd[256];
	snprintf(cmd, sizeof(cmd), "ps -o pid,stat,comm %d", pid);
	if (system(cmd))
		return 1;

	return 0;
}
