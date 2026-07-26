#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

// exec-demo.c 的演示目标程序：打印接收到的 argv 和环境变量

int main(int argc, char *argv[], char *envp[])
{
	printf("=== exec target started ===\n");

	printf("argc = %d\n", argc);
	for (int i = 0; i < argc; i++) {
		printf("argv[%d] = %s\n", i, argv[i]);
	}

	printf("--- environment variables ---\n");
	for (int i = 0; envp[i] != NULL; i++) {
		printf("envp[%d] = %s\n", i, envp[i]);
	}

	printf("=== exec target finished ===\n");

	printf("虽然，argv[0] 可以调整，comm 就是程序的名称\n");
	char cmd[256];
	snprintf(cmd, sizeof(cmd), "cat /proc/%d/comm\n", getpid());
	if (system(cmd)) {
		printf("%s failed\n", cmd);
		return 1;
	}
	return 0;
}
