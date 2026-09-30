#define _GNU_SOURCE /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main()
{
	int pipefd[2];
	char buffer[256];
	ssize_t nbytes;

	// 创建非阻塞管道
	if (pipe2(pipefd, O_NONBLOCK) == -1) {
		perror("pipe2 failed");
		exit(EXIT_FAILURE);
	}

	sleep(1000);
	// 尝试从管道读取数据（非阻塞模式）
	nbytes = read(pipefd[0], buffer, sizeof(buffer));
	if (nbytes == -1) {
		if (errno == EAGAIN) {
			printf("No data available in pipe (non-blocking)\n");
		} else {
			perror("read failed");
		}
	}

	// 向管道写入数据
	const char *message = "Hello from pipe2!";
	write(pipefd[1], message, strlen(message) + 1);
	printf("Writer sent: %s\n", message);

	// 从管道读取数据
	nbytes = read(pipefd[0], buffer, sizeof(buffer));
	if (nbytes > 0) {
		printf("Reader received: %s\n", buffer);
	}

	// 关闭文件描述符
	close(pipefd[0]);
	close(pipefd[1]);

	return 0;
}
