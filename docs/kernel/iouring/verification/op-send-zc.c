/*
 * 测试 IORING_OP_SEND_ZC
 *
 * 这不是什么革命性的东西，只是对于早就存在的 MSG_ZEROCOPY 的 io_uring 化。
 * https://www.kernel.org/doc/html/latest/networking/msg_zerocopy.html
 *
 * 只有使用的内存量大于 4k 的时候才可以
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <liburing.h>
#include <errno.h>
#include <signal.h>
#include <sys/wait.h>

#define PORT 12345
#define BUF_SIZE 4096

// 启动一个简单的 TCP 服务端（接收数据）
static pid_t start_tcp_server()
{
	pid_t pid = fork();
	if (pid != 0) {
		return pid; // 父进程返回
	}

	// 子进程：服务端
	int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd < 0) {
		perror("server socket");
		exit(1);
	}

	int opt = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("server bind");
		exit(1);
	}

	if (listen(listen_fd, 1) < 0) {
		perror("server listen");
		exit(1);
	}

	printf("Server listening on 127.0.0.1:%d\n", PORT);

	int client_fd = accept(listen_fd, NULL, NULL);
	if (client_fd < 0) {
		perror("server accept");
		exit(1);
	}

	// 接收所有数据
	char buf[BUF_SIZE];
	ssize_t n;
	size_t total = 0;
	while ((n = recv(client_fd, buf, sizeof(buf), 0)) > 0) {
		total += n;
	}
	printf("Server received %zu bytes\n", total);

	close(client_fd);
	close(listen_fd);
	exit(0);
}

int main()
{
	struct io_uring ring;
	int ret;

	// 初始化 io_uring
	ret = io_uring_queue_init(8, &ring, 0);
	if (ret < 0) {
		fprintf(stderr, "io_uring_queue_init failed: %s\n",
			strerror(-ret));
		return 1;
	}

	// 启动服务端
	pid_t server_pid = start_tcp_server();
	usleep(100000); // 等待服务端启动

	// 创建客户端 socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("client socket");
		goto cleanup;
	}

	// ⚠️ 必须启用 SO_ZEROCOPY
	int val = 1;
	if (setsockopt(sockfd, SOL_SOCKET, SO_ZEROCOPY, &val, sizeof(val)) <
	    0) {
		perror("setsockopt SO_ZEROCOPY");
		fprintf(stderr,
			"Your kernel or socket may not support SO_ZEROCOPY.\n");
		close(sockfd);
		goto cleanup;
	}

	// 连接服务端
	struct sockaddr_in addr = { 0 };
	addr.sin_family = AF_INET;
	addr.sin_port = htons(PORT);
	inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

	if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		perror("connect");
		close(sockfd);
		goto cleanup;
	}

	// 分配对齐的发送 buffer（建议页对齐）
	char *send_buf = aligned_alloc(4096, BUF_SIZE);
	if (!send_buf) {
		perror("aligned_alloc");
		close(sockfd);
		goto cleanup;
	}
	memset(send_buf, 'Z', BUF_SIZE - 1);
	send_buf[BUF_SIZE - 1] = '\n';

	printf("Sending %d bytes via IORING_OP_SEND_ZC...\n", BUF_SIZE);

	// 准备 SEND_ZC 请求
	struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
	if (!sqe) {
		fprintf(stderr, "io_uring_get_sqe failed\n");
		free(send_buf);
		close(sockfd);
		goto cleanup;
	}

	// 使用 liburing 封装函数（liburing ≥ 2.3）
	io_uring_prep_send_zc(sqe, sockfd, send_buf, BUF_SIZE, 0, 0);
	sqe->user_data = 0x1234;

	// 提交
	ret = io_uring_submit(&ring);
	if (ret != 1) {
		fprintf(stderr, "io_uring_submit failed: %d\n", ret);
		free(send_buf);
		close(sockfd);
		goto cleanup;
	}

	// 等待完成
	struct io_uring_cqe *cqe;
	ret = io_uring_wait_cqe(&ring, &cqe);
	if (ret < 0) {
		fprintf(stderr, "io_uring_wait_cqe failed: %s\n",
			strerror(-ret));
		free(send_buf);
		close(sockfd);
		goto cleanup;
	}

	printf("SEND_ZC result: %d (expected: %d)\n", cqe->res, BUF_SIZE);

	if (cqe->res == BUF_SIZE) {
		printf("✅ IORING_OP_SEND_ZC SUCCESS!\n");
	} else if (cqe->res < 0) {
		fprintf(stderr, "❌ SEND_ZC failed: %s\n", strerror(-cqe->res));
	} else {
		printf("⚠️ Partial send: %d bytes\n", cqe->res);
	}

	io_uring_cqe_seen(&ring, cqe);

	// 清理
	free(send_buf);
	close(sockfd);

cleanup:
	// 终止服务端
	kill(server_pid, SIGTERM);
	wait(NULL);
	io_uring_queue_exit(&ring);
	return 0;
}
