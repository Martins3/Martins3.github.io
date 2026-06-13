/*
 * 测试 IORING_OP_EPOLL_CTL
 *
 * 这个只是 epoll_ctl 系统调用的 io_uring 化。
 *
 * 使用方法 nc localhost 8888
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <liburing.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <assert.h>

#define PORT 8888
#define BACKLOG 128
#define QUEUE_DEPTH 1
#define MAX_EVENTS 10

void fatal(const char *msg)
{
	perror(msg);
	exit(1);
}

void set_nonblocking(int fd)
{
	int flags = fcntl(fd, F_GETFL, 0);
	if (flags == -1) {
		fatal("fcntl F_GETFL");
	}
	if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
		fatal("fcntl F_SETFL O_NONBLOCK");
	}
}

int main()
{
	struct io_uring ring;
	int ret;

	// 1. 初始化 io_uring
	if (io_uring_queue_init(QUEUE_DEPTH, &ring, 0) < 0) {
		fatal("io_uring_queue_init");
	}

	// 2. 创建 epoll 实例
	int epfd = epoll_create1(0);
	if (epfd < 0) {
		fatal("epoll_create1");
	}

	// 3. 创建并设置监听套接字
	int listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock < 0) {
		fatal("socket");
	}

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	int opt = 1;
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (bind(listen_sock, (struct sockaddr *)&serv_addr,
		 sizeof(serv_addr)) < 0) {
		fatal("bind");
	}

	if (listen(listen_sock, BACKLOG) < 0) {
		fatal("listen");
	}

	printf("服务器正在端口 %d 上监听...\n", PORT);

	// 4. 使用 io_uring 准备 EPOLL_CTL 操作
	struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
	if (!sqe) {
		fprintf(stderr, "无法获取 SQE。\n");
		exit(1);
	}

	struct epoll_event ev;
	ev.events = EPOLLIN; // 监控连接请求
	ev.data.fd = listen_sock;

	// 使用 liburing 辅助函数准备 epoll_ctl 操作
	io_uring_prep_epoll_ctl(sqe, epfd, listen_sock, EPOLL_CTL_ADD, &ev);

	// 5. 提交请求到内核
	if (io_uring_submit(&ring) < 0) {
		fatal("io_uring_submit");
	}

	// 6. 等待 epoll_ctl 操作完成
	struct io_uring_cqe *cqe;
	ret = io_uring_wait_cqe(&ring, &cqe);
	if (ret < 0) {
		fatal("io_uring_wait_cqe");
	}

	if (cqe->res < 0) {
		fprintf(stderr, "epoll_ctl 异步操作失败: %s\n",
			strerror(-cqe->res));
		exit(1);
	}
	printf("已通过 io_uring 成功将监听套接字添加到 epoll 实例。\n");
	io_uring_cqe_seen(&ring, cqe);

	// 7. 进入主事件循环 (使用传统的 epoll_wait)
	printf("设置完成，进入主事件循环。\n");
	struct epoll_event events[MAX_EVENTS];
	while (1) {
		int num_events = epoll_wait(epfd, events, MAX_EVENTS, -1);
		if (num_events < 0) {
			fatal("epoll_wait");
		}

		for (int i = 0; i < num_events; i++) {
			if (events[i].data.fd == listen_sock) {
				// 处理新连接
				int client_sock =
					accept(listen_sock, NULL, NULL);
				if (client_sock < 0) {
					perror("accept");
					continue;
				}

				set_nonblocking(
					client_sock); // 将客户端套接字设为非阻塞
				printf("接受到新连接，fd: %d\n", client_sock);

				ev.events = EPOLLIN |
					    EPOLLET; // 监控读事件，使用边缘触发
				ev.data.fd = client_sock;
				// 将新客户端添加到 epoll (这里为简化使用同步调用)
				if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock,
					      &ev) < 0) {
					fatal("epoll_ctl: add client_sock");
				}
			} else {
				// 处理客户端数据
				int client_fd = events[i].data.fd;
				char buffer[1024] = { 0 };
				int bytes_read =
					read(client_fd, buffer, sizeof(buffer));

				if (bytes_read <= 0) {
					// 客户端断开连接或出错
					printf("客户端 fd: %d 断开连接。\n",
					       client_fd);
					epoll_ctl(epfd, EPOLL_CTL_DEL,
						  client_fd, NULL);
					close(client_fd);
				} else {
					// 将收到的数据打印并回显给客户端
					printf("从 fd %d 收到消息: %s",
					       client_fd, buffer);
					int ret = write(client_fd, buffer,
							bytes_read);
					assert(ret == bytes_read);
				}
			}
		}
	}

	// 8. 清理资源 (在此示例的无限循环中不会执行到)
	close(listen_sock);
	close(epfd);
	io_uring_queue_exit(&ring);

	return 0;
}
