#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <liburing.h>

int main(int argc, char *argv[])
{
	// struct io_uring 是 liburing 抽象出来的
	printf("io_uring\t\t: %ld\n", sizeof(struct io_uring));
	printf("io_uring_sq\t\t: %ld\n", sizeof(struct io_uring_sq));
	printf("io_uring_cq\t\t: %ld\n", sizeof(struct io_uring_cq));
	printf("io_uring_sqe\t\t: %ld\n", sizeof(struct io_uring_sqe));
	printf("io_uring_cqe\t\t: %ld\n", sizeof(struct io_uring_cqe));

	// io_uring_setup() 的参数
	printf("io_uring_params\t\t: %ld\n", sizeof(struct io_uring_params));
	printf("io_sqring_offsets\t: %ld\n", sizeof(struct io_sqring_offsets));
	printf("io_cqring_offsets\t: %ld\n", sizeof(struct io_cqring_offsets));

	return EXIT_SUCCESS;
}
