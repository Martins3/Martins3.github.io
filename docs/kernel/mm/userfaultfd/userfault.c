#define _GNU_SOURCE

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <linux/memfd.h>
#include <linux/userfaultfd.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define TEST_SKIP 77

enum resolver {
	RESOLVE_COPY,
	RESOLVE_MOVE,
	RESOLVE_WP,
	RESOLVE_CONTINUE,
};

struct handler_ctx {
	int uffd;
	enum resolver resolver;
	void *region;
	size_t len;
	size_t page_size;
	void *copy_page;
	atomic_bool stop;
	atomic_int error;
	atomic_int missing_faults;
	atomic_int minor_faults;
	atomic_int wp_faults;
	atomic_int remove_events;
};

static const char *resolver_name(enum resolver resolver)
{
	switch (resolver) {
	case RESOLVE_COPY:
		return "UFFDIO_COPY";
	case RESOLVE_MOVE:
		return "UFFDIO_MOVE";
	case RESOLVE_WP:
		return "UFFDIO_WRITEPROTECT";
	case RESOLVE_CONTINUE:
		return "UFFDIO_CONTINUE";
	}

	return "unknown";
}

static void handler_error(struct handler_ctx *ctx, const char *operation)
{
	int saved_errno = errno;

	fprintf(stderr, "handler: %s: %s\n", operation, strerror(saved_errno));
	atomic_store(&ctx->error, saved_errno ? saved_errno : EIO);
}

static int open_userfaultfd(int userfaultfd_flags)
{
	int flags = O_CLOEXEC | O_NONBLOCK | userfaultfd_flags;
	int syscall_errno;
	int dev;
	int uffd;

	// TODO 按道理不应该用这个的
	uffd = syscall(__NR_userfaultfd, flags);
	if (uffd >= 0)
		return uffd;

	syscall_errno = errno;
	dev = open("/dev/userfaultfd", O_RDWR | O_CLOEXEC);
	if (dev < 0) {
		errno = syscall_errno;
		return -1;
	}

	uffd = ioctl(dev, USERFAULTFD_IOC_NEW, flags);
	close(dev);
	return uffd;
}

static int create_userfaultfd_with_flags(uint64_t required_features,
					 uint64_t *available_features,
					 int userfaultfd_flags)
{
	struct uffdio_api api = {
		.api = UFFD_API,
		.features = required_features,
	};
	int uffd;

	uffd = open_userfaultfd(userfaultfd_flags);
	if (uffd < 0) {
		if (!(userfaultfd_flags & UFFD_USER_MODE_ONLY) &&
		    (errno == EPERM || errno == EACCES)) {
			printf("SKIP: creating a userfaultfd that handles kernel "
			       "faults requires permission\n");
			return -TEST_SKIP;
		}
		fprintf(stderr, "open userfaultfd: %s\n", strerror(errno));
		return -1;
	}

	if (ioctl(uffd, UFFDIO_API, &api) < 0) {
		int saved_errno = errno;

		close(uffd);
		if (saved_errno == EINVAL && required_features) {
			printf("SKIP: kernel does not support requested features "
			       "0x%llx\n",
			       (unsigned long long)required_features);
			return -TEST_SKIP;
		}
		fprintf(stderr, "UFFDIO_API: %s\n", strerror(saved_errno));
		return -1;
	}

	if ((api.features & required_features) != required_features) {
		printf("SKIP: requested features 0x%llx, available 0x%llx\n",
		       (unsigned long long)required_features,
		       (unsigned long long)api.features);
		close(uffd);
		return -TEST_SKIP;
	}

	*available_features = api.features;
	return uffd;
}

static int create_userfaultfd(uint64_t required_features,
			      uint64_t *available_features)
{
	return create_userfaultfd_with_flags(required_features,
					  available_features,
					  UFFD_USER_MODE_ONLY);
}

static int register_range(int uffd, void *start, size_t len, uint64_t mode,
			  uint64_t required_ioctls)
{
	struct uffdio_register reg = {
		.range = {
			.start = (uintptr_t)start,
			.len = len,
		},
		.mode = mode,
	};

	if (ioctl(uffd, UFFDIO_REGISTER, &reg) < 0) {
		fprintf(stderr, "UFFDIO_REGISTER(mode=0x%llx): %s\n",
			(unsigned long long)mode, strerror(errno));
		return -1;
	}

	if ((reg.ioctls & required_ioctls) != required_ioctls) {
		fprintf(stderr,
			"registered range lacks ioctls 0x%llx (has 0x%llx)\n",
			(unsigned long long)required_ioctls,
			(unsigned long long)reg.ioctls);
		return -1;
	}

	return 0;
}

static int unregister_range(int uffd, void *start, size_t len)
{
	struct uffdio_range range = {
		.start = (uintptr_t)start,
		.len = len,
	};

	if (ioctl(uffd, UFFDIO_UNREGISTER, &range) < 0) {
		fprintf(stderr, "UFFDIO_UNREGISTER: %s\n", strerror(errno));
		return -1;
	}

	return 0;
}

static int resolve_copy(struct handler_ctx *ctx, uintptr_t address)
{
	size_t index = (address - (uintptr_t)ctx->region) / ctx->page_size;
	struct uffdio_copy copy = {
		.src = (uintptr_t)ctx->copy_page,
		.dst = address,
		.len = ctx->page_size,
	};

	memset(ctx->copy_page, 'A' + (index % 26), ctx->page_size);
	if (ioctl(ctx->uffd, UFFDIO_COPY, &copy) < 0)
		return -1;
	if (copy.copy != (long long)ctx->page_size) {
		errno = EIO;
		return -1;
	}

	return 0;
}

static int resolve_move(struct handler_ctx *ctx, uintptr_t address)
{
	struct uffdio_move move = {
		.dst = address,
		.len = ctx->page_size,
	};
	void *source;
	int ioctl_ret;
	int ret = -1;

	source = mmap(NULL, ctx->page_size, PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (source == MAP_FAILED)
		return -1;
	memset(source, 'M', ctx->page_size);
	move.src = (uintptr_t)source;

	ioctl_ret = ioctl(ctx->uffd, UFFDIO_MOVE, &move);
	if (ioctl_ret == 0) {
		if (move.move == (long long)ctx->page_size)
			ret = 0;
		else
			errno = EIO;
	}
	// TODO 这里没有对比结果，需要验证下

	// TODO 这感觉非常奇怪，既然都 move 了，为什么还有 munmap source ?
	munmap(source, ctx->page_size);
	return ret;
}

static int resolve_wp(struct handler_ctx *ctx, uintptr_t address)
{
	struct uffdio_writeprotect wp = {
		.range = {
			.start = address,
			.len = ctx->page_size,
		},
		.mode = 0,
	};

	return ioctl(ctx->uffd, UFFDIO_WRITEPROTECT, &wp);
}

static int resolve_continue(struct handler_ctx *ctx, uintptr_t address)
{
	struct uffdio_continue cont = {
		.range = {
			.start = address,
			.len = ctx->page_size,
		},
		.mode = 0,
	};

	if (ioctl(ctx->uffd, UFFDIO_CONTINUE, &cont) < 0)
		return -1;
	if (cont.mapped != (long long)ctx->page_size) {
		errno = EIO;
		return -1;
	}

	return 0;
}

static int handle_pagefault(struct handler_ctx *ctx, const struct uffd_msg *msg)
{
	uint64_t flags = msg->arg.pagefault.flags;
	uintptr_t address = msg->arg.pagefault.address & ~(ctx->page_size - 1);
	const char *kind;
	int ret;

	if (flags & UFFD_PAGEFAULT_FLAG_MINOR) {
		kind = "minor";
		atomic_fetch_add(&ctx->minor_faults, 1);
	} else if (flags & UFFD_PAGEFAULT_FLAG_WP) {
		kind = "write-protect";
		atomic_fetch_add(&ctx->wp_faults, 1);
	} else {
		kind = "missing";
		atomic_fetch_add(&ctx->missing_faults, 1);
	}

	printf("EVENT pagefault=%s address=%p flags=0x%llx resolve=%s\n", kind,
	       (void *)address, (unsigned long long)flags,
	       resolver_name(ctx->resolver));

	// TODO 他是完全按照测试类型做的，也就是我提前知道自己是
	// 不管怎么说，应该判断 ctx->resolver 和 uffd 的 event 是相同的
	// 才可以
	switch (ctx->resolver) {
	case RESOLVE_COPY:
		ret = resolve_copy(ctx, address);
		break;
	case RESOLVE_MOVE:
		ret = resolve_move(ctx, address);
		break;
	case RESOLVE_WP:
		ret = resolve_wp(ctx, address);
		break;
	case RESOLVE_CONTINUE:
		ret = resolve_continue(ctx, address);
		break;
	default:
		errno = EINVAL;
		ret = -1;
	}

	return ret;
}

static void *handler_thread(void *arg)
{
	struct handler_ctx *ctx = arg;
	struct pollfd pfd = {
		.fd = ctx->uffd,
		.events = POLLIN,
	};

	while (!atomic_load(&ctx->stop)) {
		struct uffd_msg msg;
		ssize_t bytes;
		int ready;

		ready = poll(&pfd, 1, 100);
		if (ready < 0) {
			if (errno == EINTR)
				continue;
			handler_error(ctx, "poll");
			break;
		}
		if (ready == 0)
			continue;
		if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
			errno = EIO;
			handler_error(ctx, "poll revents");
			break;
		}

		bytes = read(ctx->uffd, &msg, sizeof(msg));
		if (bytes < 0) {
			if (errno == EAGAIN || errno == EINTR)
				continue;
			handler_error(ctx, "read");
			break;
		}
		if (bytes != sizeof(msg)) {
			errno = EIO;
			handler_error(ctx, "short read");
			break;
		}

		if (msg.event == UFFD_EVENT_PAGEFAULT) {
			if (handle_pagefault(ctx, &msg) < 0) {
				handler_error(ctx,
					      resolver_name(ctx->resolver));
				break;
			}
		} else if (msg.event == UFFD_EVENT_REMOVE) {
			printf("EVENT remove start=%p end=%p\n",
			       (void *)(uintptr_t)msg.arg.remove.start,
			       (void *)(uintptr_t)msg.arg.remove.end);
			atomic_fetch_add(&ctx->remove_events, 1);
		} else {
			fprintf(stderr, "handler: unexpected event 0x%x\n",
				msg.event);
			atomic_store(&ctx->error, EPROTO);
			break;
		}
	}

	return NULL;
}

static int start_handler(struct handler_ctx *ctx, pthread_t *thread)
{
	int error;

	atomic_init(&ctx->stop, false);
	atomic_init(&ctx->error, 0);
	atomic_init(&ctx->missing_faults, 0);
	atomic_init(&ctx->minor_faults, 0);
	atomic_init(&ctx->wp_faults, 0);
	atomic_init(&ctx->remove_events, 0);

	error = pthread_create(thread, NULL, handler_thread, ctx);
	if (error) {
		fprintf(stderr, "pthread_create: %s\n", strerror(error));
		return -1;
	}

	return 0;
}

static int stop_handler(struct handler_ctx *ctx, pthread_t thread)
{
	int error;

	atomic_store(&ctx->stop, true);
	error = pthread_join(thread, NULL);
	if (error) {
		fprintf(stderr, "pthread_join: %s\n", strerror(error));
		return -1;
	}

	return atomic_load(&ctx->error) ? -1 : 0;
}

static int wait_for_counter(atomic_int *counter, int expected)
{
	struct timespec delay = {
		.tv_nsec = 10 * 1000 * 1000,
	};

	for (int i = 0; i < 100; i++) {
		if (atomic_load(counter) >= expected)
			return 0;
		nanosleep(&delay, NULL);
	}

	return -1;
}

static int write_exact(int fd, const void *buffer, size_t len)
{
	const char *bytes = buffer;

	while (len) {
		ssize_t written = write(fd, bytes, len);

		if (written < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		bytes += written;
		len -= written;
	}

	return 0;
}

static int read_exact(int fd, void *buffer, size_t len)
{
	char *bytes = buffer;

	while (len) {
		ssize_t count = read(fd, bytes, len);

		if (count < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (count == 0) {
			errno = EPIPE;
			return -1;
		}
		bytes += count;
		len -= count;
	}

	return 0;
}

static void init_ctx(struct handler_ctx *ctx, int uffd, enum resolver resolver,
		     void *region, size_t len, size_t page_size)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->uffd = uffd;
	ctx->resolver = resolver;
	ctx->region = region;
	ctx->len = len;
	ctx->page_size = page_size;
}

static int run_missing(enum resolver resolver, size_t page_size)
{
	uint64_t required_feature =
		resolver == RESOLVE_MOVE ? UFFD_FEATURE_MOVE : 0;
	uint64_t required_ioctl = resolver == RESOLVE_MOVE ?
					  (1ULL << _UFFDIO_MOVE) :
					  (1ULL << _UFFDIO_COPY);
	uint64_t features;
	struct handler_ctx ctx;
	pthread_t thread;
	volatile char *bytes;
	void *region = MAP_FAILED;
	size_t len = resolver == RESOLVE_COPY ? page_size * 2 : page_size;
	int uffd;
	int ret = 1;
	bool started = false;

	uffd = create_userfaultfd(required_feature, &features);
	if (uffd == -TEST_SKIP)
		return TEST_SKIP;
	if (uffd < 0)
		return 1;

	region = mmap(NULL, len, PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (region == MAP_FAILED) {
		fprintf(stderr, "mmap anonymous: %s\n", strerror(errno));
		goto out;
	}
	if (register_range(uffd, region, len, UFFDIO_REGISTER_MODE_MISSING,
			   required_ioctl) < 0)
		goto out;

	init_ctx(&ctx, uffd, resolver, region, len, page_size);
	if (posix_memalign(&ctx.copy_page, page_size, page_size)) {
		fprintf(stderr, "posix_memalign failed\n");
		goto unregister;
	}
	if (start_handler(&ctx, &thread) < 0)
		goto unregister;
	started = true;

	bytes = region;
	if (resolver == RESOLVE_COPY) {
		if (bytes[0] != 'A' || bytes[page_size] != 'B') {
			fprintf(stderr,
				"UFFDIO_COPY returned unexpected data\n");
			goto unregister;
		}
		if (atomic_load(&ctx.missing_faults) != 2)
			goto unregister;
	} else {
		if (bytes[0] != 'M') {
			fprintf(stderr,
				"UFFDIO_MOVE returned unexpected data\n");
			goto unregister;
		}
		if (atomic_load(&ctx.missing_faults) != 1)
			goto unregister;
	}

	ret = 0;

unregister:
	if (started && stop_handler(&ctx, thread) < 0)
		ret = 1;
	if (region != MAP_FAILED && unregister_range(uffd, region, len) < 0)
		ret = 1;
	free(ctx.copy_page);
out:
	if (region != MAP_FAILED)
		munmap(region, len);
	close(uffd);
	(void)features;
	return ret;
}

static int run_write_protect(size_t page_size)
{
	uint64_t features;
	struct uffdio_writeprotect wp;
	struct handler_ctx ctx;
	pthread_t thread;
	volatile char *byte;
	void *region = MAP_FAILED;
	int uffd;
	int ret = 1;
	bool started = false;

	uffd = create_userfaultfd(UFFD_FEATURE_PAGEFAULT_FLAG_WP, &features);
	if (uffd == -TEST_SKIP)
		return TEST_SKIP;
	if (uffd < 0)
		return 1;

	region = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (region == MAP_FAILED) {
		fprintf(stderr, "mmap anonymous: %s\n", strerror(errno));
		goto out;
	}
	memset(region, 'P', page_size);
	if (register_range(uffd, region, page_size, UFFDIO_REGISTER_MODE_WP,
			   1ULL << _UFFDIO_WRITEPROTECT) < 0)
		goto out;

	init_ctx(&ctx, uffd, RESOLVE_WP, region, page_size, page_size);
	if (start_handler(&ctx, &thread) < 0)
		goto unregister;
	started = true;

	memset(&wp, 0, sizeof(wp));
	wp.range.start = (uintptr_t)region;
	wp.range.len = page_size;
	wp.mode = UFFDIO_WRITEPROTECT_MODE_WP;
	if (ioctl(uffd, UFFDIO_WRITEPROTECT, &wp) < 0) {
		fprintf(stderr, "UFFDIO_WRITEPROTECT(WP): %s\n",
			strerror(errno));
		goto unregister;
	}

	byte = region;
	*byte = 'W';
	if (*byte != 'W' || atomic_load(&ctx.wp_faults) != 1) {
		fprintf(stderr, "write-protect fault was not handled\n");
		goto unregister;
	}
	ret = 0;

unregister:
	if (started && stop_handler(&ctx, thread) < 0)
		ret = 1;
	if (unregister_range(uffd, region, page_size) < 0)
		ret = 1;
out:
	if (region != MAP_FAILED)
		munmap(region, page_size);
	close(uffd);
	(void)features;
	return ret;
}

static int create_shmem(size_t len)
{
	int fd = memfd_create("userfaultfd-demo", MFD_CLOEXEC);

	if (fd < 0) {
		fprintf(stderr, "memfd_create: %s\n", strerror(errno));
		return -1;
	}
	if (ftruncate(fd, len) < 0) {
		fprintf(stderr, "ftruncate memfd: %s\n", strerror(errno));
		close(fd);
		return -1;
	}

	return fd;
}

static int run_remove_shmem(size_t page_size)
{
	uint64_t required_features = UFFD_FEATURE_EVENT_REMOVE |
				     UFFD_FEATURE_MISSING_SHMEM;
	uint64_t features;
	struct handler_ctx ctx;
	pthread_t thread;
	volatile char *byte;
	void *region = MAP_FAILED;
	int memfd = -1;
	int uffd;
	int ret = 1;
	bool started = false;

	uffd = create_userfaultfd(required_features, &features);
	if (uffd == -TEST_SKIP)
		return TEST_SKIP;
	if (uffd < 0)
		return 1;
	memfd = create_shmem(page_size);
	if (memfd < 0)
		goto out;

	region = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED,
		      memfd, 0);
	if (region == MAP_FAILED) {
		fprintf(stderr, "mmap shmem: %s\n", strerror(errno));
		goto out;
	}
	memset(region, 'R', page_size);
	if (register_range(uffd, region, page_size,
			   UFFDIO_REGISTER_MODE_MISSING,
			   1ULL << _UFFDIO_COPY) < 0)
		goto out;

	init_ctx(&ctx, uffd, RESOLVE_COPY, region, page_size, page_size);
	if (posix_memalign(&ctx.copy_page, page_size, page_size)) {
		fprintf(stderr, "posix_memalign failed\n");
		goto unregister;
	}
	if (start_handler(&ctx, &thread) < 0)
		goto unregister;
	started = true;

	if (madvise(region, page_size, MADV_REMOVE) < 0) {
		fprintf(stderr, "madvise(MADV_REMOVE): %s\n", strerror(errno));
		goto unregister;
	}
	if (wait_for_counter(&ctx.remove_events, 1) < 0) {
		fprintf(stderr, "timed out waiting for UFFD_EVENT_REMOVE\n");
		goto unregister;
	}

	byte = region;
	if (*byte != 'A' || atomic_load(&ctx.missing_faults) != 1) {
		fprintf(stderr,
			"missing fault after MADV_REMOVE was not handled\n");
		goto unregister;
	}
	ret = 0;

unregister:
	if (started && stop_handler(&ctx, thread) < 0)
		ret = 1;
	if (unregister_range(uffd, region, page_size) < 0)
		ret = 1;
	free(ctx.copy_page);
out:
	if (region != MAP_FAILED)
		munmap(region, page_size);
	if (memfd >= 0)
		close(memfd);
	close(uffd);
	(void)features;
	return ret;
}

static int run_minor_shmem(size_t page_size)
{
	static const char payload[] = "minor-shmem page-cache data";
	uint64_t features;
	struct handler_ctx ctx;
	pthread_t thread;
	void *alias = MAP_FAILED;
	void *region = MAP_FAILED;
	int memfd = -1;
	int uffd;
	int ret = 1;
	bool started = false;

	uffd = create_userfaultfd(UFFD_FEATURE_MINOR_SHMEM, &features);
	if (uffd == -TEST_SKIP)
		return TEST_SKIP;
	if (uffd < 0)
		return 1;
	memfd = create_shmem(page_size);
	if (memfd < 0)
		goto out;

	alias = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED, memfd,
		     0);
	region = mmap(NULL, page_size, PROT_READ | PROT_WRITE, MAP_SHARED,
		      memfd, 0);
	if (alias == MAP_FAILED || region == MAP_FAILED) {
		fprintf(stderr, "mmap shmem aliases: %s\n", strerror(errno));
		goto out;
	}

	/* Populate shmem's page cache through the unregistered alias.  The
	 * registered mapping deliberately has no PTE yet, so its first access
	 * becomes a userfaultfd minor fault rather than a missing fault. */
	memcpy(alias, payload, sizeof(payload));
	if (register_range(uffd, region, page_size, UFFDIO_REGISTER_MODE_MINOR,
			   1ULL << _UFFDIO_CONTINUE) < 0)
		goto out;

	init_ctx(&ctx, uffd, RESOLVE_CONTINUE, region, page_size, page_size);
	if (start_handler(&ctx, &thread) < 0)
		goto unregister;
	started = true;

	if (memcmp(region, payload, sizeof(payload)) != 0) {
		fprintf(stderr, "shmem data changed across UFFDIO_CONTINUE\n");
		goto unregister;
	}
	if (atomic_load(&ctx.minor_faults) != 1 ||
	    atomic_load(&ctx.missing_faults) != 0) {
		fprintf(stderr,
			"expected one minor fault and no missing faults\n");
		goto unregister;
	}
	ret = 0;

unregister:
	if (started && stop_handler(&ctx, thread) < 0)
		ret = 1;
	if (unregister_range(uffd, region, page_size) < 0)
		ret = 1;
out:
	if (alias != MAP_FAILED)
		munmap(alias, page_size);
	if (region != MAP_FAILED)
		munmap(region, page_size);
	if (memfd >= 0)
		close(memfd);
	close(uffd);
	(void)features;
	return ret;
}

struct gdb_target_ready {
	uintptr_t address;
	int status;
};

struct gdb_target_result {
	short uffd_revents;
	int error;
};

static void run_gdb_target(int ready_fd, int control_fd, size_t page_size,
			   int userfaultfd_flags)
{
	struct gdb_target_ready ready = {
		.status = 1,
	};
	struct gdb_target_result result = { 0 };
	uint64_t features;
	struct pollfd pfd;
	void *region = MAP_FAILED;
	char command;
	int uffd = -1;
	int status = 1;

	uffd = create_userfaultfd_with_flags(0, &features, userfaultfd_flags);
	if (uffd == -TEST_SKIP) {
		ready.status = TEST_SKIP;
		goto report_ready;
	}
	if (uffd < 0)
		goto report_ready;

	region = mmap(NULL, page_size, PROT_READ | PROT_WRITE,
		      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (region == MAP_FAILED) {
		fprintf(stderr, "gdb target mmap: %s\n", strerror(errno));
		goto report_ready;
	}
	if (register_range(uffd, region, page_size,
			   UFFDIO_REGISTER_MODE_MISSING,
			   1ULL << _UFFDIO_COPY) < 0)
		goto report_ready;

	/* Allow the sibling GDB process through Yama ptrace_scope. */
	if (prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY) < 0) {
		fprintf(stderr, "prctl(PR_SET_PTRACER): %s\n", strerror(errno));
		goto unregister;
	}

	ready.address = (uintptr_t)region;
	ready.status = 0;
report_ready:
	if (write_exact(ready_fd, &ready, sizeof(ready)) < 0)
		goto unregister;
	if (ready.status)
		goto out;

	if (read_exact(control_fd, &command, sizeof(command)) < 0)
		goto unregister;

	pfd.fd = uffd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	if (poll(&pfd, 1, 0) < 0)
		result.error = errno;
	else
		result.uffd_revents = pfd.revents;
	if (write_exact(ready_fd, &result, sizeof(result)) < 0)
		goto unregister;
	status = 0;

unregister:
	if (region != MAP_FAILED && uffd >= 0 &&
	    unregister_range(uffd, region, page_size) < 0)
		status = 1;
out:
	if (region != MAP_FAILED)
		munmap(region, page_size);
	if (uffd >= 0)
		close(uffd);
	close(ready_fd);
	close(control_fd);
	(void)features;
	_exit(ready.status ? ready.status : status);
}

static int wait_with_timeout(pid_t pid, int *status)
{
	struct timespec delay = {
		.tv_nsec = 10 * 1000 * 1000,
	};

	for (int i = 0; i < 1000; i++) {
		pid_t waited = waitpid(pid, status, WNOHANG);

		if (waited == pid)
			return 0;
		if (waited < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		nanosleep(&delay, NULL);
	}

	errno = ETIMEDOUT;
	return -1;
}

static int run_gdb(pid_t target, uintptr_t address, char *output,
		   size_t output_size)
{
	char pid_arg[32];
	char examine_command[64];
	int output_pipe[2];
	int status;
	pid_t gdb;
	size_t used = 0;

	if (pipe2(output_pipe, O_CLOEXEC) < 0) {
		fprintf(stderr, "pipe2(gdb output): %s\n", strerror(errno));
		return 1;
	}

	gdb = fork();
	if (gdb < 0) {
		fprintf(stderr, "fork(gdb): %s\n", strerror(errno));
		close(output_pipe[0]);
		close(output_pipe[1]);
		return 1;
	}
	if (gdb == 0) {
		close(output_pipe[0]);
		if (dup2(output_pipe[1], STDOUT_FILENO) < 0 ||
		    dup2(output_pipe[1], STDERR_FILENO) < 0)
			_exit(126);
		close(output_pipe[1]);
		if (setenv("LC_ALL", "C", 1) < 0)
			_exit(126);
		snprintf(pid_arg, sizeof(pid_arg), "%jd", (intmax_t)target);
		snprintf(examine_command, sizeof(examine_command),
			 "x/1bx 0x%" PRIxPTR, address);
		execlp("gdb", "gdb", "--quiet", "--nx", "--batch", "--pid",
		       pid_arg, "--ex", examine_command, (char *)NULL);
		_exit(errno == ENOENT ? 127 : 126);
	}

	close(output_pipe[1]);
	if (wait_with_timeout(gdb, &status) < 0) {
		fprintf(stderr, "gdb timed out: %s\n", strerror(errno));
		kill(gdb, SIGKILL);
		waitpid(gdb, &status, 0);
		close(output_pipe[0]);
		return 1;
	}

	while (used + 1 < output_size) {
		ssize_t count = read(output_pipe[0], output + used,
				     output_size - used - 1);

		if (count < 0) {
			if (errno == EINTR)
				continue;
			break;
		}
		if (count == 0)
			break;
		used += count;
	}
	output[used] = '\0';
	close(output_pipe[0]);

	if (WIFEXITED(status) && WEXITSTATUS(status) == 127)
		return TEST_SKIP;
	if (!WIFEXITED(status)) {
		fprintf(stderr, "gdb terminated abnormally\n");
		return 1;
	}

	return 0;
}

static int run_gdb_missing_with_flags(size_t page_size,
				      int userfaultfd_flags)
{
	struct gdb_target_ready ready;
	struct gdb_target_result result;
	char output[8192] = { 0 };
	int ready_pipe[2] = { -1, -1 };
	int control_pipe[2] = { -1, -1 };
	int target_status;
	int gdb_result = 1;
	int ret = 1;
	pid_t target;
	char command = 'q';

	if (pipe2(ready_pipe, O_CLOEXEC) < 0) {
		fprintf(stderr, "pipe2(gdb target): %s\n", strerror(errno));
		return 1;
	}
	if (pipe2(control_pipe, O_CLOEXEC) < 0) {
		fprintf(stderr, "pipe2(gdb control): %s\n", strerror(errno));
		goto close_pipes;
	}

	target = fork();
	if (target < 0) {
		fprintf(stderr, "fork(gdb target): %s\n", strerror(errno));
		goto close_pipes;
	}
	if (target == 0) {
		close(ready_pipe[0]);
		close(control_pipe[1]);
		run_gdb_target(ready_pipe[1], control_pipe[0], page_size,
			       userfaultfd_flags);
	}

	close(ready_pipe[1]);
	ready_pipe[1] = -1;
	close(control_pipe[0]);
	control_pipe[0] = -1;
	if (read_exact(ready_pipe[0], &ready, sizeof(ready)) < 0) {
		fprintf(stderr, "read(gdb target ready): %s\n", strerror(errno));
		goto stop_target;
	}
	if (ready.status) {
		ret = ready.status;
		goto stop_target;
	}

	gdb_result = run_gdb(target, ready.address, output, sizeof(output));
	if (write_exact(control_pipe[1], &command, sizeof(command)) < 0) {
		fprintf(stderr, "release(gdb target): %s\n", strerror(errno));
		goto stop_target;
	}
	if (read_exact(ready_pipe[0], &result, sizeof(result)) < 0) {
		fprintf(stderr, "read(gdb target result): %s\n", strerror(errno));
		goto stop_target;
	}
	if (gdb_result == TEST_SKIP) {
		printf("SKIP: gdb is not installed\n");
		ret = TEST_SKIP;
		goto stop_target;
	}
	if (gdb_result)
		goto show_output;
	if (!strstr(output, "Cannot access memory at address")) {
		fprintf(stderr, "gdb did not report an inaccessible address\n");
		goto show_output;
	}
	if (result.error) {
		fprintf(stderr, "poll(userfaultfd): %s\n",
			strerror(result.error));
		goto stop_target;
	}
	if (result.uffd_revents) {
		fprintf(stderr,
			"unexpected userfaultfd event after GDB access: 0x%x\n",
			(unsigned int)result.uffd_revents);
		goto stop_target;
	}
	printf("GDB: Cannot access memory at address 0x%" PRIxPTR "\n",
	       ready.address);
	ret = 0;
	goto stop_target;

show_output:
	fprintf(stderr, "gdb output:\n%s", output);
stop_target:
	close(control_pipe[1]);
	control_pipe[1] = -1;
	if (wait_with_timeout(target, &target_status) < 0) {
		kill(target, SIGKILL);
		waitpid(target, &target_status, 0);
		ret = 1;
	} else if (ret == 0 &&
		   (!WIFEXITED(target_status) || WEXITSTATUS(target_status))) {
		fprintf(stderr, "gdb target exited abnormally\n");
		ret = 1;
	}

close_pipes:
	if (ready_pipe[0] >= 0)
		close(ready_pipe[0]);
	if (ready_pipe[1] >= 0)
		close(ready_pipe[1]);
	if (control_pipe[0] >= 0)
		close(control_pipe[0]);
	if (control_pipe[1] >= 0)
		close(control_pipe[1]);
	return ret;
}

static int run_gdb_missing(size_t page_size)
{
	return run_gdb_missing_with_flags(page_size, UFFD_USER_MODE_ONLY);
}

static int run_gdb_missing_no_user_mode_only(size_t page_size)
{
	return run_gdb_missing_with_flags(page_size, 0);
}

struct test_case {
	const char *name;
	int (*run)(size_t page_size);
};

static int run_missing_copy(size_t page_size)
{
	return run_missing(RESOLVE_COPY, page_size);
}

static int run_missing_move(size_t page_size)
{
	return run_missing(RESOLVE_MOVE, page_size);
}

static const struct test_case tests[] = {
	{ "missing-copy", run_missing_copy },
	{ "missing-move", run_missing_move },
	{ "write-protect", run_write_protect },
	{ "remove-shmem", run_remove_shmem },
	{ "minor-shmem", run_minor_shmem },
	{ "gdb-missing", run_gdb_missing },
	{ "gdb-missing-no-user-mode-only",
	  run_gdb_missing_no_user_mode_only },
};

static void usage(const char *program)
{
	fprintf(stderr, "Usage: %s [all|TEST]\n\nTests:\n", program);
	for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++)
		fprintf(stderr, "  %s\n", tests[i].name);
}

int main(int argc, char **argv)
{
	const char *selected = argc == 1 ? "all" : argv[1];
	long page_size = sysconf(_SC_PAGESIZE);
	int failures = 0;
	int skipped = 0;
	int executed = 0;

	if (argc > 2 || page_size <= 0) {
		usage(argv[0]);
		return 2;
	}

	for (size_t i = 0; i < sizeof(tests) / sizeof(tests[0]); i++) {
		int result;

		if (strcmp(selected, "all") && strcmp(selected, tests[i].name))
			continue;
		executed++;
		printf("\nTEST %s\n", tests[i].name);
		result = tests[i].run((size_t)page_size);
		if (result == 0) {
			printf("PASS %s\n", tests[i].name);
		} else if (result == TEST_SKIP) {
			printf("SKIP %s\n", tests[i].name);
			skipped++;
		} else {
			printf("FAIL %s\n", tests[i].name);
			failures++;
		}
	}

	// TODO 这个几个也需要看看是什么意思?
	// UFFD_FEATURE_WP_UNPOPULATED
	// UFFD_FEATURE_WP_HUGETLBFS_SHMEM
	// UFFD_FEATURE_EXACT_ADDRESS
	// UFFD_FEATURE_WP_ASYNC
	// UFFDIO_COPY_MODE_DONTWAKE
	if (!executed) {
		usage(argv[0]);
		return 2;
	}

	printf("\nSUMMARY: %d passed, %d skipped, %d failed\n",
	       executed - skipped - failures, skipped, failures);
	if (failures)
		return 1;
	if (skipped == executed)
		return TEST_SKIP;
	return 0;
}
