#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <limits.h>
#include <linux/aio_abi.h>
#include <linux/io_uring.h>
#include <linux/magic.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/vfs.h>
#include <time.h>
#include <unistd.h>

#define BUFFER_SIZE 4096U
#define DEFAULT_ITERATIONS 10000U

/*
 * O_DIRECT makes the non-fixed paths pin the userspace page for DMA.  Since
 * every I/O completes before the next submission, a one-page buffer should
 * add one acquired and one released FOLL_PIN reference per iteration.  A
 * fixed buffer should be pinned once at registration, reused by every I/O,
 * and released at unregistration.
 *
 * The test file must live on a filesystem that performs real direct I/O.
 * tmpfs accepts O_DIRECT but does not build block-layer bios, so it cannot
 * demonstrate per-I/O page pinning.  Use --wait when attaching a tracer to
 * the process by PID.
 *
 * One way to count pin operations is:
 *
 *   @[
        pin_user_pages_fast+5
        iov_iter_extract_pages+266
        iov_iter_extract_bvecs+116
        bio_iov_iter_get_pages+144
        iomap_dio_bio_iter_one+443
        iomap_dio_bio_iter+321
        __iomap_dio_rw+1017
        iomap_dio_rw+18
        xfs_file_dio_read+233
        xfs_file_read_iter+267
        __io_read+193
        io_read+63
        __io_issue_sqe+59
        io_issue_sqe+55
        io_submit_sqe+83
        io_submit_sqes+220
        __do_sys_io_uring_enter+532
        do_syscall_64+226
        entry_SYSCALL_64_after_hwframe+118
]: 10000

如果执行 --fixed 的，那么就只有这一次
@[
        pin_user_pages_fast+5
        io_pin_pages+157
        io_sqe_buffer_register+218
        io_sqe_buffers_register.cold+417
        __x64_sys_io_uring_register+149
        do_syscall_64+226
        entry_SYSCALL_64_after_hwframe+118
]: 1

至此，应该所有的问题都比较清晰了吧，也就是说，
就算是相同的内存，已经被 pin 过，如果没有 IORING_OP_READ_FIXED
，也会反复调用 pin_user_pages_fast ，因为不能保证页面是否已经被 swap out 掉。
 */

enum engine {
	ENGINE_AIO,
	ENGINE_URING,
	ENGINE_URING_FIXED,
};

struct raw_ring {
	int fd;
	struct io_uring_params params;
	void *sq_ring;
	size_t sq_ring_size;
	void *cq_ring;
	size_t cq_ring_size;
	struct io_uring_sqe *sqes;
	size_t sqes_size;
	unsigned *sq_tail;
	unsigned *sq_mask;
	unsigned *sq_array;
	unsigned *cq_head;
	unsigned *cq_tail;
	unsigned *cq_mask;
	struct io_uring_cqe *cqes;
};

static void usage(const char *program)
{
	fprintf(stderr,
		"Usage: %s [--wait] [--directory DIR] "
		"aio|uring|uring-fixed [iterations]\n",
		program);
}

static void report_error(const char *operation, int error)
{
	fprintf(stderr, "%s: %s\n", operation, strerror(error));
}

static long read_vmpin_kb(void)
{
	char line[256];
	long value = -1;
	FILE *status = fopen("/proc/self/status", "r");

	if (!status)
		return -1;
	while (fgets(line, sizeof(line), status)) {
		if (sscanf(line, "VmPin: %ld kB", &value) == 1)
			break;
	}
	fclose(status);
	return value;
}

static int wait_for_tracer(void)
{
	int character;

	printf("waiting before measured phases: pid=%ld "
	       "(press Enter to continue)\n",
	       (long)getpid());
	fflush(stdout);
	do {
		character = getchar();
	} while (character != '\n' && character != EOF);
	return ferror(stdin) ? -EIO : 0;
}

static double elapsed_seconds(const struct timespec *start,
			      const struct timespec *end)
{
	return (double)(end->tv_sec - start->tv_sec) +
	       (double)(end->tv_nsec - start->tv_nsec) / 1000000000.0;
}

static int prepare_direct_file(const char *directory)
{
	struct statfs file_system;
	char path[PATH_MAX];
	unsigned char data[BUFFER_SIZE];
	ssize_t written;
	int direct_fd;
	int fd;
	int length;

	if (statfs(directory, &file_system) < 0)
		return -errno;
	if ((unsigned long)file_system.f_type == TMPFS_MAGIC)
		return -EOPNOTSUPP;
	length = snprintf(path, sizeof(path), "%s/.register-buffer-pin-XXXXXX",
			  directory);
	if (length < 0 || (size_t)length >= sizeof(path))
		return -ENAMETOOLONG;

	memset(data, 0x5a, sizeof(data));
	fd = mkstemp(path);
	if (fd < 0)
		return -errno;

	written = pwrite(fd, data, sizeof(data), 0);
	if (written != (ssize_t)sizeof(data)) {
		int error = written < 0 ? errno : EIO;

		close(fd);
		unlink(path);
		return -error;
	}
	if (fdatasync(fd) < 0) {
		int error = errno;

		close(fd);
		unlink(path);
		return -error;
	}
	close(fd);

	direct_fd = open(path, O_RDONLY | O_DIRECT);
	if (direct_fd < 0) {
		int error = errno;

		unlink(path);
		return -error;
	}
	if (unlink(path) < 0) {
		int error = errno;

		close(direct_fd);
		return -error;
	}
	return direct_fd;
}

static int run_aio(int fd, void *buffer, unsigned iterations)
{
	aio_context_t context = 0;
	struct io_event event;
	struct iocb control_block;
	struct iocb *list[] = { &control_block };
	unsigned i;
	long result;
	int ret;

	result = syscall(SYS_io_setup, 1, &context);
	if (result < 0)
		return -errno;

	for (i = 0; i < iterations; i++) {
		memset(&control_block, 0, sizeof(control_block));
		control_block.aio_lio_opcode = IOCB_CMD_PREAD;
		control_block.aio_fildes = fd;
		control_block.aio_buf = (uintptr_t)buffer;
		control_block.aio_nbytes = BUFFER_SIZE;
		control_block.aio_offset = 0;

		result = syscall(SYS_io_submit, context, 1, list);
		if (result != 1) {
			ret = result < 0 ? -errno : -EIO;
			goto out;
		}
		result = syscall(SYS_io_getevents, context, 1, 1, &event, NULL);
		if (result != 1) {
			ret = result < 0 ? -errno : -EIO;
			goto out;
		}
		if (event.res != BUFFER_SIZE) {
			ret = event.res < 0 ? (int)event.res : -EIO;
			goto out;
		}
	}
	ret = 0;
out:
	if (syscall(SYS_io_destroy, context) < 0 && ret == 0)
		ret = -errno;
	return ret;
}

static void destroy_ring(struct raw_ring *ring)
{
	if (ring->sqes && ring->sqes != MAP_FAILED)
		munmap(ring->sqes, ring->sqes_size);
	if (ring->cq_ring && ring->cq_ring != MAP_FAILED &&
	    ring->cq_ring != ring->sq_ring)
		munmap(ring->cq_ring, ring->cq_ring_size);
	if (ring->sq_ring && ring->sq_ring != MAP_FAILED)
		munmap(ring->sq_ring, ring->sq_ring_size);
	if (ring->fd >= 0)
		close(ring->fd);
	memset(ring, 0, sizeof(*ring));
	ring->fd = -1;
}

static int setup_ring(struct raw_ring *ring)
{
	size_t ring_size;

	memset(ring, 0, sizeof(*ring));
	ring->fd = -1;
	ring->fd = syscall(SYS_io_uring_setup, 2, &ring->params);
	if (ring->fd < 0)
		return -errno;

	ring->sq_ring_size = ring->params.sq_off.array +
			     ring->params.sq_entries * sizeof(unsigned);
	ring->cq_ring_size =
		ring->params.cq_off.cqes +
		ring->params.cq_entries * sizeof(struct io_uring_cqe);
	if (ring->params.features & IORING_FEAT_SINGLE_MMAP) {
		ring_size = ring->sq_ring_size > ring->cq_ring_size ?
				    ring->sq_ring_size :
				    ring->cq_ring_size;
		ring->sq_ring_size = ring_size;
		ring->cq_ring_size = ring_size;
	}

	ring->sq_ring = mmap(NULL, ring->sq_ring_size, PROT_READ | PROT_WRITE,
			     MAP_SHARED | MAP_POPULATE, ring->fd,
			     IORING_OFF_SQ_RING);
	if (ring->sq_ring == MAP_FAILED)
		goto mmap_error;

	if (ring->params.features & IORING_FEAT_SINGLE_MMAP) {
		ring->cq_ring = ring->sq_ring;
	} else {
		ring->cq_ring = mmap(NULL, ring->cq_ring_size,
				     PROT_READ | PROT_WRITE,
				     MAP_SHARED | MAP_POPULATE, ring->fd,
				     IORING_OFF_CQ_RING);
		if (ring->cq_ring == MAP_FAILED)
			goto mmap_error;
	}

	ring->sqes_size = ring->params.sq_entries * sizeof(*ring->sqes);
	ring->sqes = mmap(NULL, ring->sqes_size, PROT_READ | PROT_WRITE,
			  MAP_SHARED | MAP_POPULATE, ring->fd, IORING_OFF_SQES);
	if (ring->sqes == MAP_FAILED)
		goto mmap_error;

	ring->sq_tail = ring->sq_ring + ring->params.sq_off.tail;
	ring->sq_mask = ring->sq_ring + ring->params.sq_off.ring_mask;
	ring->sq_array = ring->sq_ring + ring->params.sq_off.array;
	ring->cq_head = ring->cq_ring + ring->params.cq_off.head;
	ring->cq_tail = ring->cq_ring + ring->params.cq_off.tail;
	ring->cq_mask = ring->cq_ring + ring->params.cq_off.ring_mask;
	ring->cqes = ring->cq_ring + ring->params.cq_off.cqes;
	return 0;

mmap_error: {
	int error = errno;

	destroy_ring(ring);
	return -error;
}
}

static int submit_uring_read(struct raw_ring *ring, int fd, void *buffer,
			     int fixed)
{
	struct io_uring_cqe *cqe;
	struct io_uring_sqe *sqe;
	unsigned cq_head;
	unsigned sq_tail;
	int ret;

	sq_tail = atomic_load_explicit((_Atomic unsigned *)ring->sq_tail,
				       memory_order_relaxed);
	sqe = &ring->sqes[sq_tail & *ring->sq_mask];
	memset(sqe, 0, sizeof(*sqe));
	sqe->opcode = fixed ? IORING_OP_READ_FIXED : IORING_OP_READ;
	sqe->fd = fd;
	sqe->off = 0;
	sqe->addr = (uintptr_t)buffer;
	sqe->len = BUFFER_SIZE;
	if (fixed)
		sqe->buf_index = 0;
	ring->sq_array[sq_tail & *ring->sq_mask] = sq_tail & *ring->sq_mask;
	atomic_store_explicit((_Atomic unsigned *)ring->sq_tail, sq_tail + 1,
			      memory_order_release);

	ret = syscall(SYS_io_uring_enter, ring->fd, 1, 1,
		      IORING_ENTER_GETEVENTS, NULL, 0);
	if (ret < 0)
		return -errno;

	cq_head = atomic_load_explicit((_Atomic unsigned *)ring->cq_head,
				       memory_order_acquire);
	if (cq_head == atomic_load_explicit((_Atomic unsigned *)ring->cq_tail,
					    memory_order_acquire))
		return -EIO;
	cqe = &ring->cqes[cq_head & *ring->cq_mask];
	ret = cqe->res == BUFFER_SIZE ? 0 : (cqe->res < 0 ? cqe->res : -EIO);
	atomic_store_explicit((_Atomic unsigned *)ring->cq_head, cq_head + 1,
			      memory_order_release);
	return ret;
}

static int run_uring(int fd, void *buffer, unsigned iterations, int fixed)
{
	struct raw_ring ring;
	struct iovec iovec = {
		.iov_base = buffer,
		.iov_len = BUFFER_SIZE,
	};
	unsigned i;
	int ret;

	ret = setup_ring(&ring);
	if (ret < 0)
		return ret;

	if (fixed) {
		printf("VmPin before register: %ld kB\n", read_vmpin_kb());
		ret = syscall(SYS_io_uring_register, ring.fd,
			      IORING_REGISTER_BUFFERS, &iovec, 1);
		if (ret < 0) {
			ret = -errno;
			goto out;
		}
		printf("VmPin after register:  %ld kB\n", read_vmpin_kb());
	}

	for (i = 0; i < iterations; i++) {
		ret = submit_uring_read(&ring, fd, buffer, fixed);
		if (ret < 0)
			goto unregister;
	}
	ret = 0;

unregister:
	if (fixed) {
		printf("VmPin after I/O:       %ld kB\n", read_vmpin_kb());
		if (syscall(SYS_io_uring_register, ring.fd,
			    IORING_UNREGISTER_BUFFERS, NULL, 0) < 0 &&
		    ret == 0)
			ret = -errno;
		printf("VmPin after unregister: %ld kB\n", read_vmpin_kb());
	}
out:
	destroy_ring(&ring);
	return ret;
}

int main(int argc, char **argv)
{
	static const struct option long_options[] = {
		{ "wait", no_argument, NULL, 'w' },
		{ "directory", required_argument, NULL, 'd' },
		{ "help", no_argument, NULL, 'h' },
		{ NULL, 0, NULL, 0 },
	};
	struct timespec start;
	struct timespec end;
	unsigned iterations = DEFAULT_ITERATIONS;
	unsigned char *buffer;
	char *end_pointer;
	unsigned long parsed;
	enum engine engine;
	const char *directory = ".";
	const char *engine_name;
	double seconds;
	int wait = 0;
	int option;
	int fd;
	int ret;

	while ((option = getopt_long(argc, argv, "wd:h", long_options, NULL)) !=
	       -1) {
		switch (option) {
		case 'w':
			wait = 1;
			break;
		case 'd':
			directory = optarg;
			break;
		case 'h':
			usage(argv[0]);
			return EXIT_SUCCESS;
		default:
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}
	if (optind >= argc || argc - optind > 2) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	engine_name = argv[optind++];
	if (!strcmp(engine_name, "aio"))
		engine = ENGINE_AIO;
	else if (!strcmp(engine_name, "uring"))
		engine = ENGINE_URING;
	else if (!strcmp(engine_name, "uring-fixed"))
		engine = ENGINE_URING_FIXED;
	else {
		usage(argv[0]);
		return EXIT_FAILURE;
	}

	if (optind < argc) {
		errno = 0;
		parsed = strtoul(argv[optind], &end_pointer, 10);
		if (errno || *end_pointer || parsed == 0 ||
		    parsed > UINT32_MAX) {
			fprintf(stderr, "invalid iteration count: %s\n",
				argv[optind]);
			return EXIT_FAILURE;
		}
		iterations = (unsigned)parsed;
	}

	ret = prctl(PR_SET_NAME,
		    engine == ENGINE_AIO   ? "pin-aio" :
		    engine == ENGINE_URING ? "pin-uring" :
					     "pin-uring-fix",
		    0, 0, 0);
	if (ret < 0) {
		report_error("prctl", errno);
		return EXIT_FAILURE;
	}
	ret = posix_memalign((void **)&buffer, BUFFER_SIZE, BUFFER_SIZE);
	if (ret) {
		report_error("posix_memalign", ret);
		return EXIT_FAILURE;
	}
	memset(buffer, 0, BUFFER_SIZE);

	fd = prepare_direct_file(directory);
	if (fd < 0) {
		fprintf(stderr, "O_DIRECT test directory: %s\n", directory);
		report_error("prepare O_DIRECT file", -fd);
		free(buffer);
		return EXIT_FAILURE;
	}

	printf("engine=%s iterations=%u block=%u bytes pid=%ld\n", engine_name,
	       iterations, BUFFER_SIZE, (long)getpid());
	printf("O_DIRECT directory=%s\n", directory);
	if (engine != ENGINE_URING_FIXED)
		printf("VmPin before I/O: %ld kB\n", read_vmpin_kb());
	if (wait) {
		ret = wait_for_tracer();
		if (ret < 0) {
			report_error("wait for tracer", -ret);
			close(fd);
			free(buffer);
			return EXIT_FAILURE;
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &start);
	if (engine == ENGINE_AIO)
		ret = run_aio(fd, buffer, iterations);
	else
		ret = run_uring(fd, buffer, iterations,
				engine == ENGINE_URING_FIXED);
	clock_gettime(CLOCK_MONOTONIC, &end);
	if (engine != ENGINE_URING_FIXED)
		printf("VmPin after I/O:  %ld kB\n", read_vmpin_kb());

	if (ret < 0) {
		report_error("I/O", -ret);
		close(fd);
		free(buffer);
		return EXIT_FAILURE;
	}
	if (buffer[0] != 0x5a) {
		fprintf(stderr, "data verification failed\n");
		close(fd);
		free(buffer);
		return EXIT_FAILURE;
	}

	seconds = elapsed_seconds(&start, &end);
	printf("elapsed=%.6f s iops=%.0f\n", seconds, iterations / seconds);
	close(fd);
	free(buffer);
	return EXIT_SUCCESS;
}
