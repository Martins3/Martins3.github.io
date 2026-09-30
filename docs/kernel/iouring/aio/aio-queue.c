// 分析一下 aio 的共享内存
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h> // syscall numbers
#include <linux/aio_abi.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <inttypes.h>
#include <assert.h>
#include <fcntl.h>

// uses the kernel-based aio syscalls directly
// (NOT libc's aio(7) that use threads)

// syscall wrappers
static inline int io_setup(unsigned maxevents, aio_context_t *ctx)
{
	return syscall(SYS_io_setup, maxevents, ctx);
}

static inline int io_submit(aio_context_t ctx, long nr, struct iocb **iocbpp)
{
	return syscall(SYS_io_submit, ctx, nr, iocbpp);
}

static inline int io_getevents(aio_context_t ctx, long min_nr, long nr,
			       struct io_event *events,
			       struct timespec *timeout)
{
	return syscall(SYS_io_getevents, ctx, min_nr, nr, events, timeout);
}

static inline int io_destroy(aio_context_t ctx)
{
	return syscall(SYS_io_destroy, ctx);
}

struct aio_ring {
	unsigned id; /* kernel internal index number */
	unsigned nr; /* number of io_events */
	unsigned head; /* Written to by userland or under ring_lock
				 * mutex by aio_read_events_ring(). */
	unsigned tail;

	unsigned magic;
	unsigned compat_features;
	unsigned incompat_features;
	unsigned header_length; /* size of aio_ring */

	struct io_event io_events[];
}; /* 128 bytes + ring size */

static void show_kernel_msg(aio_context_t t)
{
	struct aio_ring *my_ring = (struct aio_ring *)t;
	printf("aio_ring information:\n");
	printf("  id:             %u\n", my_ring->id);
	printf("  nr:              %u\n", my_ring->nr);
	printf("  head:            %u\n", my_ring->head);
	printf("  tail:            %u\n", my_ring->tail);
	printf("  magic:           %u\n", my_ring->magic);
	printf("  compat_features: %u\n", my_ring->compat_features);
	printf("  incompat_features: %u\n", my_ring->incompat_features);
	printf("  header_length:  %u\n", my_ring->header_length);
}

int main(int argc, char *argv[])
{
	aio_context_t ioctx = 0;
	unsigned maxevents = 128;

	int fd = open("./aio.c", O_RDONLY | O_DIRECT);
	if (fd == -1) {
		perror(argv[1]);
		exit(1);
	}

	if (io_setup(maxevents, &ioctx) < 0) {
		perror("io_setup");
		exit(1);
	}

#define BUF_LEN 1024
	// first operation
	char buff1[BUF_LEN];
	memset(buff1, 0, sizeof(buff1));
	struct iocb iocb1 = { 0 };
	iocb1.aio_data = 0xbeef; /* will be returned in events data */
	iocb1.aio_fildes = fd;
	iocb1.aio_lio_opcode = IOCB_CMD_PREAD;
	iocb1.aio_reqprio = 0;
	iocb1.aio_buf = (uintptr_t)buff1;
	iocb1.aio_nbytes = sizeof(buff1);
	iocb1.aio_offset = 0;

	// second operation
	char buff2[BUF_LEN];
	memset(buff2, 0, sizeof(buff2));
	struct iocb iocb2 = { 0 };
	iocb2.aio_data = 0xbaba; /* will be returned in events data */
	iocb2.aio_fildes = fd;
	iocb2.aio_lio_opcode = IOCB_CMD_PREAD;
	iocb2.aio_reqprio = 0;
	iocb2.aio_buf = (uintptr_t)buff2;
	iocb2.aio_nbytes = sizeof(buff2);
	iocb2.aio_offset = BUF_LEN;

	struct iocb *iocb_ptrs[2] = { &iocb1, &iocb2 };

	// submit operations
	int ret = io_submit(ioctx, 2, iocb_ptrs);
	if (ret < 0) {
		perror("io_submit");
		exit(1);
	} else if (ret != 2) {
		perror("io_submit: unhandled partial success");
		exit(1);
	}

	/*
	 * 正如从内核中看到的一样，实际上，内核传递给用户态的指针是一个地址，
	 * 然后通过这个地址可以修改 aio_ring 。
	 */
#ifdef HACKING_IOCTX
	struct aio_ring *r = (struct aio_ring *)ioctx;
	r->head = 2;
	r->tail = 10;
#endif
	show_kernel_msg(ioctx);

	size_t nevents = 2;
	struct io_event events[2];
	while (nevents > 0) {
		// wait for at least one event
		ret = io_getevents(ioctx, 1 /* min */, nevents, events, NULL);
		if (ret < 0) {
			perror("io_getevents");
			exit(1);
		}

		for (size_t i = 0; i < ret; i++) {
			struct io_event *ev = &events[i];
			/* assert(ev->data == 0xbeef || ev->data == 0xbaba); */
			printf("Event returned with res=%lld res2=%lld\n",
			       ev->res, ev->res2);
			nevents--;
		}
	}

	buff1[sizeof(buff1) - 1] = '\0';
	printf("%s\n", buff1);
	printf("------------------------------\n");
	buff2[sizeof(buff2) - 1] = '\0';
	printf("%s\n", buff2);

	show_kernel_msg(ioctx);
	/*
	 * [ioctx:main:160] 7fb2e9e62000
	 *
	 * 7fb2e9e62000-7fb2e9e65000 rw-s 00000000 00:11 4078826                    /[aio] (deleted)
	 *
	 * 的确是
	 */
	printf("[ioctx:%s:%d] %lx\n", __FUNCTION__, __LINE__, ioctx);
	/* sleep(1000); */

	io_destroy(ioctx);
	close(fd);
}
