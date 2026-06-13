/*
 * 测试这几个功能:
 * IORING_REGISTER_EVENTFD_ASYNC
 * IORING_REGISTER_EVENTFD
 * IORING_UNREGISTER_EVENTFD
 *
 * 整个效果也很简单，注册了 eventfd 之后，当 uring 中的
 * 工作完成了，那么也回去通知一下 eventfd 
 *
 * 具体实现 io_uring/eventfd.c:io_eventfd_signal
 *
 * 参考 https://nick-black.com/dankwiki/index.php/Io_uring
 * 为了可以同时兼容 epoll 和 io_uring 可以有如下三个方法:
 * 1. Add the epoll fd to your uring with IORING_OP_POLL_ADD (ideally multishot), 
 *	and wait only for uring readiness. When you get a CQE for this submitted
 *	event, check the epoll.
 * 2. Poll on the uring's file descriptor directly for completion queue events.
 * 3. Register an eventfd with your uring with io_uring_register_eventfd(3), add
 *	that to your epoll, and when you get POLLIN for this fd, check the
 *	completion ring.
 *
 * 第一个方法，也就是通过 io_uring_prep_poll_add 让 io_uring 来监听 epoll fd 
 *
 * 第二种方法，也就是 work-with-ppoll.c 中演示的，让 epoll 直接监听
 * io_uring::ring_fd 。QEMU 就是使用的这种方法。
 *
 * 第三种方法，当 iouring 完成之后，eventfd 上存在事件，那么就可以让
 * io_uring 被管理起来。
 */

#include <sys/eventfd.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <liburing.h>
#include <fcntl.h>

#define BUFF_SZ 512

char buff[BUFF_SZ + 1];
struct io_uring ring;

void error_exit(char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

void *listener_thread(void *data)
{
	struct io_uring_cqe *cqe;
	int efd = *(int *)data;
	eventfd_t v;
	printf("%s: Waiting for completion event...\n", __FUNCTION__);

	int ret = eventfd_read(efd, &v);
	if (ret < 0)
		error_exit("eventfd_read");

	printf("%s: Got completion event. Counter value: %llu\n", __FUNCTION__,
	       (unsigned long long)v);

	ret = io_uring_wait_cqe(&ring, &cqe);
	if (ret < 0) {
		fprintf(stderr, "Error waiting for completion: %s\n",
			strerror(-ret));
		return NULL;
	}
	/* Process the completion queue entry */
	if (cqe->res < 0) {
		fprintf(stderr, "Error in async operation: %s\n",
			strerror(-cqe->res));
	}
	printf("Result of the operation: %d\n", cqe->res);
	io_uring_cqe_seen(&ring, cqe);

	printf("Contents read from file:\n%s\n", buff);
	return NULL;
}

int setup_io_uring(int efd)
{
	int ret = io_uring_queue_init(8, &ring, 0);
	if (ret) {
		fprintf(stderr, "Unable to setup io_uring: %s\n",
			strerror(-ret));
		return 1;
	}

	// 使用同步注册 (IORING_REGISTER_EVENTFD)
	ret = io_uring_register_eventfd(&ring, efd);
	// 或者使用异步注册 (IORING_REGISTER_EVENTFD_ASYNC，只对异步完成触发通知)
	// ret = io_uring_register_eventfd_async(&ring, efd);
	if (ret) {
		fprintf(stderr, "Unable to register eventfd: %s\n",
			strerror(-ret));
		io_uring_queue_exit(&ring);
		return 1;
	}
	printf("Eventfd registered successfully.\n");
	return 0;
}

int read_file_with_io_uring()
{
	struct io_uring_sqe *sqe;

	sqe = io_uring_get_sqe(&ring);
	if (!sqe) {
		fprintf(stderr, "Could not get SQE.\n");
		return 1;
	}

	int fd = open("/etc/passwd", O_RDONLY);
	if (fd < 0)
		error_exit("open");
	io_uring_prep_read(sqe, fd, buff, BUFF_SZ, 0);
	io_uring_submit(&ring);
	return 0;
}

int main()
{
	pthread_t t;
	int efd;

	/* Create an eventfd instance */
	efd = eventfd(0, 0);
	if (efd < 0)
		error_exit("eventfd");

	/* Create the listener thread, passing the eventfd */
	pthread_create(&t, NULL, listener_thread, &efd);

	for (size_t i = 0; i < 4; i++) {
		printf(".\n");
		sleep(1);
	}
	/* Setup io_uring and register eventfd */
	if (setup_io_uring(efd)) {
		return 1;
	}

	/* Initiate a read operation with io_uring */
	if (read_file_with_io_uring()) {
		return 1;
	}

	pthread_join(t, NULL);

	int ret = io_uring_unregister_eventfd(&ring);
	if (ret) {
		fprintf(stderr, "Unable to unregister eventfd: %s\n",
			strerror(-ret));
	} else {
		printf("Eventfd unregistered successfully.\n");
	}

	io_uring_queue_exit(&ring);
	close(efd);
	return EXIT_SUCCESS;
}
