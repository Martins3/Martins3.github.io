/*
 * IORING_OP_EPOLL_WAIT 是 2025-02-20 添加的
 *
 * commit 19f7e9427327 ("io_uring/epoll: add support for IORING_OP_EPOLL_WAIT")
 *
 * 首先，iouring 的改造是逐步进行的，很多系统，例如 qemu 中同时存在 epoll 和 iouring
 * 那么如何混合两个模式可以这样:
 *
 *   模式A：io_uring fd → epoll（老方式）
 *
 *  [应用程序]
 *      ↓
 *  [epoll_wait()] ← 监视 [io_uring fd] + [其他 legacy fd]
 *      ↓
 *    处理事件
 *
 *  问题：
 *
 *  1. epoll_wait 功能受限 - 不支持 partial reaping（部分收割）、批量等待
 *  2. 性能损耗 - io_uring 检测到"被 epoll 监视"后会激活特殊逻辑（如额外同步），导致变慢
 *
 *  新模式：epoll → io_uring（新方式）
 *
 *  [应用程序]
 *      ↓
 *  [io_uring_wait()] ← 统一处理 [io_uring CQE] + [epoll 事件]
 *      ↓
 *    处理事件
 *
 * 所以，现在通过 io_uring_prep_epoll_wait 就可以让 ioruing 来监听 epfd 了
 *
 * 使用方法: nc localhost 8888
 */

#define _GNU_SOURCE
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
#include <errno.h>

#define PORT 8888
#define BACKLOG 128
#define QUEUE_DEPTH 256
#define MAX_EVENTS 32

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

// 处理客户端断开连接
void handle_client_close(struct io_uring *ring, int epfd, int client_fd)
{
	printf("客户端 fd: %d 断开连接\n", client_fd);
	close(client_fd);
}

int main()
{
	struct io_uring ring;
	int ret;

	// 1. 初始化 io_uring
	ret = io_uring_queue_init(QUEUE_DEPTH, &ring, 0);
	if (ret < 0) {
		fprintf(stderr, "io_uring_queue_init failed: %d\n", ret);
		exit(1);
	}

	// 2. 创建 epoll 实例
	int epfd = epoll_create1(EPOLL_CLOEXEC);
	if (epfd < 0) {
		fatal("epoll_create1");
	}

	// 3. 创建并设置监听套接字
	int listen_sock = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (listen_sock < 0) {
		fatal("socket");
	}

	int opt = 1;
	setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(PORT);
	serv_addr.sin_addr.s_addr = INADDR_ANY;

	if (bind(listen_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		fatal("bind");
	}

	if (listen(listen_sock, BACKLOG) < 0) {
		fatal("listen");
	}

	printf("服务器正在端口 %d 上监听...\n", PORT);
	printf("使用 io_uring 执行 epoll_wait 操作\n\n");

	// 4. 使用传统 epoll_ctl 将监听套接字添加到 epoll
	// (IORING_OP_EPOLL_CTL 添加/删除操作也可以通过 io_uring 完成，但这里简化处理)
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = listen_sock;
	if (epoll_ctl(epfd, EPOLL_CTL_ADD, listen_sock, &ev) < 0) {
		fatal("epoll_ctl: add listen_sock");
	}

	printf("监听套接字已添加到 epoll 实例\n");
	printf("进入主事件循环 (使用 IORING_OP_EPOLL_WAIT)...\n\n");

	// 5. 主事件循环
	while (1) {
		// 提交 epoll_wait 请求到 io_uring
		struct epoll_event events[MAX_EVENTS];
		struct io_uring_sqe *sqe = io_uring_get_sqe(&ring);
		if (!sqe) {
			fprintf(stderr, "无法获取 SQE\n");
			exit(1);
		}

		// 使用 io_uring_prep_epoll_wait 准备异步 epoll_wait 操作
		// 参数: sqe, epoll_fd, events 数组, maxevents, flags
		io_uring_prep_epoll_wait(sqe, epfd, events, MAX_EVENTS, 0);

		ret = io_uring_submit(&ring);
		if (ret < 0) {
			fatal("io_uring_submit (epoll_wait)");
		}

		// 等待 epoll_wait 完成
		struct io_uring_cqe *cqe;
		ret = io_uring_wait_cqe(&ring, &cqe);
		if (ret < 0) {
			fatal("io_uring_wait_cqe");
		}

		int num_events = cqe->res;
		io_uring_cqe_seen(&ring, cqe);

		if (num_events < 0) {
			fprintf(stderr, "epoll_wait 失败: %s\n", strerror(-num_events));
			continue;
		}

		if (num_events == 0) {
			// 超时情况
			continue;
		}

		printf("epoll_wait 返回 %d 个事件\n", num_events);

		// 处理所有事件
		for (int i = 0; i < num_events; i++) {
			int fd = events[i].data.fd;
			uint32_t ev_flags = events[i].events;

			if (fd == listen_sock) {
				// 处理新连接
				while (1) {
					int client_sock = accept4(listen_sock, NULL, NULL,
								 SOCK_NONBLOCK | SOCK_CLOEXEC);
					if (client_sock < 0) {
						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							break;
						}
						perror("accept");
						break;
					}

					printf("接受新连接, fd: %d\n", client_sock);

					// 将客户端添加到 epoll
					struct epoll_event client_ev;
					client_ev.events = EPOLLIN | EPOLLET;
					client_ev.data.fd = client_sock;
					if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_sock, &client_ev) < 0) {
						perror("epoll_ctl: add client");
						close(client_sock);
					}
				}
			} else {
				// 处理客户端数据
				if (ev_flags & (EPOLLERR | EPOLLHUP)) {
					printf("客户端 fd: %d 出错或挂断\n", fd);
					epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
					close(fd);
					continue;
				}

				if (ev_flags & EPOLLIN) {
					char buffer[1024];
					ssize_t bytes_read = read(fd, buffer, sizeof(buffer));

					if (bytes_read <= 0) {
						if (bytes_read < 0 &&
						    (errno == EAGAIN || errno == EWOULDBLOCK)) {
							continue;
						}
						printf("客户端 fd: %d 断开连接\n", fd);
						epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
						close(fd);
					} else {
						printf("从 fd %d 收到 %zd 字节: %.*s",
						       fd, bytes_read, (int)bytes_read,
						       buffer);
						// 回显数据
						ssize_t written = write(fd, buffer, bytes_read);
						(void)written;
					}
				}
			}
		}
	}

	// 清理资源 (在实际运行中不会执行到这里)
	close(listen_sock);
	close(epfd);
	io_uring_queue_exit(&ring);

	return 0;
}
