/*
 * 这里演示一个 sigpipe 的经典例子，当写入 socket 的时候，
 * 如果对端已经关闭了，那么会接受到一个 sigpipe 信号。
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <signal.h>
#include <errno.h>

#define SOCKET_PATH "/tmp/sigpipe_demo_socket"

void sigpipe_handler(int sig)
{
	printf("Received SIGPIPE! (errno=%d)\n", errno);
	exit(1);
}

int main()
{
	struct sockaddr_un addr;
	int sock_fd, client_fd;
	pid_t pid;

	// 可选：设置 SIGPIPE 处理函数（注释掉则默认终止进程）
	// signal(SIGPIPE, sigpipe_handler);
	signal(SIGPIPE, SIG_IGN);

	// 创建 socket
	if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		perror("socket");
		exit(1);
	}

	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

	// 绑定
	unlink(SOCKET_PATH); // 移除旧 socket 文件
	if (bind(sock_fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
		perror("bind");
		exit(1);
	}

	if (listen(sock_fd, 1) == -1) {
		perror("listen");
		exit(1);
	}

	pid = fork();
	if (pid == 0) {
		// 等待 server 先运行
		sleep(1);
		int client_sock = socket(AF_UNIX, SOCK_STREAM, 0);
		if (connect(client_sock, (struct sockaddr *)&addr,
			    sizeof(addr)) == -1) {
			perror("client connect");
			exit(1);
		}
		close(client_fd);
		exit(0);
	} else if (pid > 0) {
		if ((client_fd = accept(sock_fd, NULL, NULL)) == -1) {
			perror("accept");
			exit(1);
		}

		sleep(1);

		// 此时 client 已关闭，再写就会触发 SIGPIPE
		if (write(client_fd, "hello", 5) < 0) {
			printf("[martins3:%s:%d] %d\n", __FUNCTION__, __LINE__, errno);
			perror("write");
			exit(1);
		}
	} else {
		perror("fork");
		exit(1);
	}

	return 0;
}
