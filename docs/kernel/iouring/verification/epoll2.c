/*
 * deepseek 生成，测试 io_uring_prep_poll_add
 * 也就是 对应这两个功能:
 *
 * IORING_OP_POLL_ADD,
 * IORING_OP_POLL_REMOVE,
 *
 * 从这里例子看 iouring 是如何实现 epoll 的功能的，
 * 算是 util/fdmon-io_uring.c 功能的模拟了。
 *
 * 使用 io_uring_prep_poll_add 功能的确有点傻，
 * 每次要监听之前都是添加一下所有的 fd 。
 *
 * io_uring_prep_poll_multishot 就不需要重复添加了。
 *
 * 使用方法 nc localhost 8888
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>
#include <liburing.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8888
#define BACKLOG 128
#define QUEUE_DEPTH 256
#define BUFFER_SIZE 1024

// 定义事件类型
enum event_type {
	EVENT_TYPE_POLL_ACCEPT,
	EVENT_TYPE_POLL_READ,
};

// 关联请求状态的数据结构
struct request {
	enum event_type type;
	int fd;
};

void fatal(const char *msg)
{
	perror(msg);
	exit(1);
}

// 提交一个新的 poll 请求
void add_poll_request(struct io_uring *ring, int fd, enum event_type type)
{
	// 为每个连接分配独立的 request 结构
	struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
	struct request *req = malloc(sizeof(*req));
	req->type = type;
	req->fd = fd;

	io_uring_prep_poll_add(sqe, fd, POLLIN);
	io_uring_sqe_set_data(sqe, req);
}

int main()
{
	struct io_uring ring;

	// 1. 初始化 io_uring 实例
	if (io_uring_queue_init(QUEUE_DEPTH, &ring, 0) < 0) {
		fatal("io_uring_queue_init");
	}

	// 2. 创建并设置监听套接字
	int listen_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
	if (listen_fd < 0)
		fatal("socket");

	int opt = 1;
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in serv_addr;
	memset(&serv_addr, 0, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(PORT);

	if (bind(listen_fd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <
	    0)
		fatal("bind");
	if (listen(listen_fd, BACKLOG) < 0)
		fatal("listen");

	printf("Poll-based Echo server listening on port %d\n", PORT);

	// 3. 提交第一个 poll 请求来监控新连接
	add_poll_request(&ring, listen_fd, EVENT_TYPE_POLL_ACCEPT);

	// 4. 主事件循环
	while (1) {
		int ret = io_uring_submit_and_wait(&ring, 1);
		if (ret < 0)
			fatal("io_uring_submit_and_wait");

		struct io_uring_cqe *cqe;
		unsigned head;
		unsigned count = 0;

		io_uring_for_each_cqe(&ring, head, cqe)
		{
			count++;
			struct request *req =
				(struct request *)io_uring_cqe_get_data(cqe);

			if (cqe->res < 0) {
				fprintf(stderr, "Poll operation failed: %s\n",
					strerror(-cqe->res));
				close(req->fd);
				free(req);
				continue;
			}

			switch (req->type) {
			case EVENT_TYPE_POLL_ACCEPT:
				// 重新武装对监听套接字的监控
				add_poll_request(&ring, listen_fd,
						 EVENT_TYPE_POLL_ACCEPT);
				free(req); // 释放旧的 accept poll 请求

				// 因为 poll 是水平触发的，一个事件可能意味着多个连接已准备好
				while (1) {
					int client_fd =
						accept(listen_fd, NULL, NULL);
					if (client_fd < 0)
						break; // 没有更多连接了

					printf("Accepted new connection, fd: %d\n",
					       client_fd);
					// 为新连接提交第一个读事件监控
					add_poll_request(&ring, client_fd,
							 EVENT_TYPE_POLL_READ);
				}
				break;

			case EVENT_TYPE_POLL_READ: {
				char buffer[BUFFER_SIZE];
				int bytes_read =
					read(req->fd, buffer, BUFFER_SIZE);

				if (bytes_read <= 0) {
					printf("Client fd %d disconnected.\n",
					       req->fd);
					close(req->fd);
					free(req); // 连接关闭，释放请求
				} else {
					printf("Read %d bytes from fd %d, echoing back.\n",
					       bytes_read, req->fd);
					write(req->fd, buffer,
					      bytes_read); // 同步回写
					// 重新武装对该客户端的读事件监控
					add_poll_request(&ring, req->fd,
							 EVENT_TYPE_POLL_READ);
					free(req); // 释放旧的 read poll 请求
				}
				break;
			}
			}
		}
		io_uring_cq_advance(&ring, count);
	}

	close(listen_fd);
	io_uring_queue_exit(&ring);
	return 0;
}
