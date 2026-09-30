/*
 * 编译：cc -Wall -Wextra -O2 orphan-printf.c -o orphan-printf.out
 * 运行：./orphan-printf.out
 *
 * 父进程直接退出，不 wait；shell 返回提示符后可以继续输入命令，
 * 子进程仍通过继承的 stdout 向同一个终端输出，每秒一行，20 秒后退出。
 *
 * 终端需要关闭 TOSTOP（通常默认关闭），才能允许后台进程写终端。
 *
 * 通过这个例子，我想展示的是，由于 child 还是在用当前的 pts ，相当于 parent 退出后，
 * bash 和 child 都是在用当前的 pts
 */
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(void)
{
	pid_t pid = fork();

	if (pid < 0) {
		perror("fork");
		return 1;
	}
	if (pid > 0) {
		printf("parent PID=%ld exits; child PID=%ld keeps printing\n",
		       (long)getpid(), (long)pid);
		return 0;
	}

	for (int i = 1; i <= 20; i++) {
		sleep(1);
		printf("child PID=%ld PPID=%ld: tick %d\n",
		       (long)getpid(), (long)getppid(), i);
		fflush(stdout);
	}
	return 0;
}
