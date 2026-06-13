/*
 * 演示一个例子:
 * 1. 只有 STOP 信号可以让程序停下来，之后通过 CONT 回复，中间 read 系统调用会自动的恢复。
 * 2. 其他常见 signal ，如果没有注册对应的 signal handler ，程序都是会直接结束的。
 *
 * man signal(7) 中提到:
 *    Interruption of system calls and library functions by signal handlers
 * 那么还有几个 signal  是不用注册 signal handler 的，记录在 docs/kernel/signal/doc.md
 * 中，仔细看，他们都是有特点的，也就是发现他们要么可以自动被忽略，要么和 STOP CONT 有关。
 *
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

int main(void)
{
	char buf[128];
	ssize_t n;

	printf("My PID is %d\n", getpid());
	printf("Run cmds in another terminal\n");
	printf("kill -STOP %d\n", getpid());
	printf("kill -CONT %d\n", getpid());

	printf("kill -USR1 %d\n", getpid());

	n = read(STDIN_FILENO, buf, sizeof(buf));

	if (n == -1) {
		perror("read");
		printf("errno = %d (%s)\n", errno, strerror(errno));
	} else {
		printf("Read %zd bytes\n", n);
	}

	return 0;
}
