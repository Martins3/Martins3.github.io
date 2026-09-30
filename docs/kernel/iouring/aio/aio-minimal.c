/*
 * 不依赖 libaio 的 aio demo
 * 注意:
 * 1. posix_memalign 来实现页面对齐
 * 2. O_DIRECT 打开
 */
#define _GNU_SOURCE
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include <fcntl.h>
#include <linux/aio_abi.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <errno.h>
#include <errno.h>
#include <stdlib.h>

inline int io_setup(unsigned nr, aio_context_t *ctxp)
{
	return syscall(__NR_io_setup, nr, ctxp);
}

inline int io_destroy(aio_context_t ctx)
{
	return syscall(__NR_io_destroy, ctx);
}

inline int io_submit(aio_context_t ctx, long nr, struct iocb **iocbpp)
{
	return syscall(__NR_io_submit, ctx, nr, iocbpp);
}

inline int io_getevents(aio_context_t ctx, long min_nr, long max_nr,
			struct io_event *events, struct timespec *timeout)
{
	return syscall(__NR_io_getevents, ctx, min_nr, max_nr, events, timeout);
}

#define DATA_BLOCK_LEN 4096
char *IO_FILE = "/dev/sda";
int main(int argc, char *argv[])
{
	aio_context_t ctx;
	struct iocb cb;
	struct iocb *cbs[1];
	void *data;
	struct io_event events[1];
	int ret;
	int fd;

	if (argv[1] != NULL)
		IO_FILE = argv[1];
	printf("test on [%s]\n", IO_FILE);

	ret = posix_memalign(&data, DATA_BLOCK_LEN, DATA_BLOCK_LEN);
	if (ret != 0) {
		perror("posix_memalign");
		return -1;
	}

	fd = open(IO_FILE, O_RDONLY | O_DIRECT);
	if (fd < 0) {
		perror("open");
		return -1;
	}

	ctx = 0;

	ret = io_setup(128, &ctx);
	if (ret < 0) {
		perror("io_setup");
		return -1;
	}

        memset(&cb, 0, sizeof(cb));
	cb.aio_fildes = fd;
	cb.aio_lio_opcode = IOCB_CMD_PREAD;

        int i;
	for (i = 0; i < DATA_BLOCK_LEN; ++i)
		*((char *)data + i) = 'A';
	cb.aio_buf = (uint64_t)data;
        cb.aio_buf = 0x100000;
        cb.aio_offset = 0;
	cb.aio_nbytes = DATA_BLOCK_LEN;

	cbs[0] = &cb;

	ret = io_submit(ctx, 1, cbs);
	if (ret != 1) {
		if (ret < 0)
			perror("io_submit");
		else
			fprintf(stderr, "io_submit failed\n");
		return -1;
	}

	ret = io_getevents(ctx, 1, 1, events, NULL);
	printf("events: %d\n", ret);
	if (events[0].res != DATA_BLOCK_LEN) {
		printf("%s", strerror(-events[0].res));
		exit(1);
	}

	ret = io_destroy(ctx);
	if (ret < 0) {
		perror("io_destroy");
		return -1;
	}
	for (i = 0; i < DATA_BLOCK_LEN; ++i)
		putchar(*((char *)data + i));
	return 0;
}
