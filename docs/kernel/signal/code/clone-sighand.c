/*
 * 测试 CLONE_SIGHAND 的效果
 *
 * CLONE_SIGHAND 表示子进程和父进程共享 signal handler table。
 * 因此，在子进程中调用 sigaction() 修改某个信号的处理函数，
 * 父进程中对应信号的处理函数也会被修改。
 *
 * 注意：Linux 要求 CLONE_SIGHAND 必须和 CLONE_VM 一起使用。
 *
 * 不过我这里没有仔细分析，为什么要这么设计，
 * 感觉 thread 就共享，process 就不要共享，就很简单了。
 */
#define _GNU_SOURCE
#include <sched.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#define STACK_SIZE 65536

/* 共享变量（依赖 CLONE_VM） */
static volatile int handler_called = 0;
static volatile int child_ready = 0;

static void old_handler(int sig)
{
	(void)sig;
	/* 默认什么都不做 */
}

static void new_handler(int sig)
{
	(void)sig;
	handler_called = 1;
}

static int child_func(void *arg)
{
	struct sigaction sa;

	(void)arg;
	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = new_handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGUSR1, &sa, NULL) == -1) {
		perror("child sigaction");
		return 1;
	}

	child_ready = 1;
	while (1)
		sleep(1);
	return 0;
}

static void test_clone_sighand(int use_sighand)
{
	char *stack = malloc(STACK_SIZE);
	unsigned long flags = CLONE_VM;
	pid_t pid;

	if (!stack) {
		perror("malloc");
		return;
	}

	if (use_sighand)
		flags |= CLONE_SIGHAND;

	handler_called = 0;
	child_ready = 0;

	pid = clone(child_func, stack + STACK_SIZE, flags | SIGCHLD, NULL);
	if (pid == -1) {
		perror("clone");
		free(stack);
		return;
	}

	/* 等待子进程设置好 signal handler */
	while (!child_ready)
		usleep(1000);

	/* 父进程向自己发送 SIGUSR1 */
	if (kill(getpid(), SIGUSR1) == -1) {
		perror("kill");
		goto cleanup;
	}

	/* 给内核留出处理信号的时间 */
	sleep(1);

	printf("CLONE_SIGHAND=%s -> handler_called=%d\n",
	       use_sighand ? "yes" : "no", handler_called);

cleanup:
	kill(pid, SIGKILL);
	waitpid(pid, NULL, 0);
	free(stack);
}

int main(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = old_handler;
	sigemptyset(&sa.sa_mask);
	if (sigaction(SIGUSR1, &sa, NULL) == -1) {
		perror("sigaction");
		return EXIT_FAILURE;
	}

	printf("PID: %d\n", getpid());

	test_clone_sighand(0);
	test_clone_sighand(1);

	return 0;
}
