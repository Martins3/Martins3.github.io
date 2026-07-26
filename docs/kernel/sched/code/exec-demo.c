/*
 * 测试验证一个我的一个小疑点，第一个参数是不是必须是程序名称
 * 答案是，这只是规定，实际上可以随便修改的。
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
	// execv / execve 演示：如何用 argv 传递参数，用 envp 传递环境变量
	//
	// int execv(const char *pathname, char *const argv[]);
	// int execve(const char *pathname, char *const argv[], char *const envp[]);
	//
	// argv[0] 通常是被执行程序的名字（这里用 "demo"），argv 数组必须以 NULL 结尾。
	// envp 数组同样必须以 NULL 结尾；execve 会用这个数组完全替换当前进程的环境变量。

	pid_t pid = fork();

	if (pid == -1) {
		perror("fork");
		return 1;
	}

	if (pid == 0) {
		// 子进程：构造 argv 和 envp，然后执行 ./exec-target.out
		char *const argv[] = {
			"program-name-can-change", // argv[0]，程序名
			"hello", // argv[1]
			"world", // argv[2]
			"from execve", // argv[3]
			NULL
		};

		char *const envp[] = { "MY_VAR=sched_demo", "LANG=C", NULL };

		// 如果不需要自定义环境变量，可用 execv：
		// execv("./exec-target.out", argv);

		execve("./exec-target.out", argv, envp);

		// execve 成功时不会返回；只有失败才会走到这里
		perror("execve");
		exit(127);
	}

	// 父进程等待子进程结束
	int status;
	pid_t wait_pid = waitpid(pid, &status, 0);

	if (wait_pid == -1) {
		perror("waitpid");
		return 1;
	}

	if (WIFEXITED(status)) {
		printf("Child exited with status: %d\n", WEXITSTATUS(status));
	} else {
		printf("Child terminated abnormally\n");
	}

	return 0;
}
