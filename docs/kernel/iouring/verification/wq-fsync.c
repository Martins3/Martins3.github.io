/*
 * 对于 fsync 这种天生不容易异步的工作，io uring 的做法
 * 是创建一个 io-wq 出来等待工作的完成
 *
 * 这里循环 10 次，可以知道内核中的
 *
 * 那么这样的话，岂不是所谓的无法异步的进行的，
 * 都是可以通过 io thread 实现?
 *
 * 不会 page cache 也是用的这种方法吧。
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <liburing.h>

static void do_sync(struct io_uring *ring, int fd)
{
	int ret;
	struct io_uring_sqe *sqe = io_uring_get_sqe(ring);
	if (!sqe) {
		fprintf(stderr, "io_uring_get_sqe failed\n");
		exit(EXIT_FAILURE);
	}

	// IORING_OP_FSYNC: 同步文件数据和元数据
	// flags: 0 -> sync data + metadata
	//        IORING_FSYNC_DATASYNC -> only data (like fdatasync)
	io_uring_prep_fsync(sqe, fd, 0); // 0 = full fsync

	// 可选：设置用户数据用于识别完成事件
	sqe->user_data = 123;

	ret = io_uring_submit(ring);
	if (ret < 0) {
		fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
		exit(EXIT_FAILURE);
	}

	struct io_uring_cqe *cqe;
	ret = io_uring_wait_cqe(ring, &cqe);
	if (ret < 0) {
		fprintf(stderr, "io_uring_wait_cqe: %s\n", strerror(-ret));
		exit(EXIT_FAILURE);
	}

	if (cqe->res < 0) {
		fprintf(stderr, "fsync failed: %s\n", strerror(-cqe->res));
	} else {
		printf("fsync completed successfully! user_data=%lu\n",
		       (unsigned long)cqe->user_data);
	}

	io_uring_cqe_seen(ring, cqe);
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	const char *filename = argv[1];

	// 1. 打开文件（必须可写，fsync 需要写权限）
	int fd = open(filename, O_RDWR | O_CREAT, 0644);
	if (fd < 0) {
		perror("open");
		exit(EXIT_FAILURE);
	}

	// 2. 写入一些数据（确保有内容可 sync）
	const char *data = "Hello, io_uring fsync!\n";
	ssize_t written = write(fd, data, strlen(data));
	if (written < 0) {
		perror("write");
		exit(EXIT_FAILURE);
	}

	struct io_uring ring;
	int ret = io_uring_queue_init(8, &ring, 0);
	if (ret < 0) {
		fprintf(stderr, "io_uring_queue_init: %s\n", strerror(-ret));
		exit(EXIT_FAILURE);
	}

	/*
	 * FIXME 提交放到一个 thread 不就完成了，为什么需要放到
	 * 构建出来这么多 thread 来。
	 */
	unsigned int workers[] = { 0, 0 };
	int io_worker = io_uring_register_iowq_max_workers(&ring, workers);
	printf("io workers default %d", io_worker);
	workers[0] = 10;
	workers[1] = 10;
	ret = io_uring_register_iowq_max_workers(&ring, workers);
	if (ret < 0) {
		perror("io_uring_register_iowq_max_workers");
		exit(1);
	}

	/*
	 * 这里故意循环 10 次，可以发现只会创建一次
	 *
	 * @[
	 *        create_io_thread+5
	 *        create_io_worker+191
	 *        io_wq_enqueue+446
	 *        io_submit_sqes+668
	 *        __do_sys_io_uring_enter+520
	 *        do_syscall_64+132
	 *        entry_SYSCALL_64_after_hwframe+118
	 * ]: 1
	 */
	for (size_t i = 0; i < 10; i++) {
		do_sync(&ring, fd);
	}

	return 0;
}
