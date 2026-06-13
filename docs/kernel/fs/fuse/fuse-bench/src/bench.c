#include "common.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

enum bench_mode {
	MODE_READ,
	MODE_WRITE,
	MODE_RW,
};

struct bench_options {
	const char *path;
	uint64_t size;
	uint64_t block_size;
	enum bench_mode mode;
	int direct;
};

static void usage(FILE *out)
{
	fprintf(out,
		"usage: bench.out --path FILE --size N[k|m|g] --bs N[k|m|g] --mode read|write|rw [--direct]\n");
}

static int parse_mode(const char *text, enum bench_mode *mode)
{
	if (strcmp(text, "read") == 0) {
		*mode = MODE_READ;
		return 0;
	}
	if (strcmp(text, "write") == 0) {
		*mode = MODE_WRITE;
		return 0;
	}
	if (strcmp(text, "rw") == 0) {
		*mode = MODE_RW;
		return 0;
	}
	return -1;
}

static int parse_args(int argc, char **argv, struct bench_options *opts)
{
	memset(opts, 0, sizeof(*opts));
	opts->block_size = 4096;
	opts->mode = MODE_READ;

	for (int i = 1; i < argc; i++) {
		if (strcmp(argv[i], "--path") == 0 && i + 1 < argc) {
			opts->path = argv[++i];
		} else if (strcmp(argv[i], "--size") == 0 && i + 1 < argc) {
			if (fb_parse_size(argv[++i], &opts->size) != 0)
				return -1;
		} else if (strcmp(argv[i], "--bs") == 0 && i + 1 < argc) {
			if (fb_parse_size(argv[++i], &opts->block_size) != 0)
				return -1;
		} else if (strcmp(argv[i], "--mode") == 0 && i + 1 < argc) {
			if (parse_mode(argv[++i], &opts->mode) != 0)
				return -1;
		} else if (strcmp(argv[i], "--direct") == 0) {
			opts->direct = 1;
		} else if (strcmp(argv[i], "-h") == 0 ||
			   strcmp(argv[i], "--help") == 0) {
			usage(stdout);
			exit(0);
		} else {
			return -1;
		}
	}

	if (!opts->path || opts->size == 0 || opts->block_size == 0)
		return -1;
	if (opts->direct && opts->block_size % 4096 != 0)
		return -1;
	return 0;
}

static const char *mode_name(enum bench_mode mode)
{
	switch (mode) {
	case MODE_READ:
		return "read";
	case MODE_WRITE:
		return "write";
	case MODE_RW:
		return "rw";
	}
	return "unknown";
}

static int do_full_io(int fd, void *buf, size_t len, off_t off, int write_op)
{
	char *p = buf;
	size_t done = 0;

	while (done < len) {
		ssize_t n;

		if (write_op)
			n = pwrite(fd, p + done, len - done, off + (off_t)done);
		else
			n = pread(fd, p + done, len - done, off + (off_t)done);
		if (n < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}
		if (n == 0)
			return -1;
		done += (size_t)n;
	}
	return 0;
}

static int run_pass(int fd, void *buf, const struct bench_options *opts,
		    int write_op)
{
	for (uint64_t off = 0; off < opts->size; off += opts->block_size) {
		uint64_t left = opts->size - off;
		size_t len = left < opts->block_size ? (size_t)left :
						       (size_t)opts->block_size;

		if (do_full_io(fd, buf, len, (off_t)off, write_op) != 0)
			return -1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	struct bench_options opts;
	void *buf = NULL;
	int fd = -1;
	int flags = O_RDWR | O_CREAT;
	uint64_t start;
	uint64_t elapsed;
	uint64_t bytes;
	int ret = 1;

	if (parse_args(argc, argv, &opts) != 0) {
		usage(stderr);
		return 2;
	}

	if (opts.direct)
		flags |= O_DIRECT;
	fd = open(opts.path, flags, 0644);
	if (fd < 0) {
		perror("open");
		return 1;
	}
	if (ftruncate(fd, (off_t)opts.size) != 0) {
		perror("ftruncate");
		goto out;
	}

	if (posix_memalign(&buf, 4096, (size_t)opts.block_size) != 0) {
		fprintf(stderr, "posix_memalign failed\n");
		goto out;
	}
	memset(buf, 0x5a, (size_t)opts.block_size);

	start = fb_now_nsec();
	if ((opts.mode == MODE_WRITE || opts.mode == MODE_RW) &&
	    run_pass(fd, buf, &opts, 1) != 0) {
		perror("write pass");
		goto out;
	}
	if ((opts.mode == MODE_READ || opts.mode == MODE_RW) &&
	    run_pass(fd, buf, &opts, 0) != 0) {
		perror("read pass");
		goto out;
	}
	elapsed = fb_now_nsec() - start;
	bytes = opts.size * (opts.mode == MODE_RW ? 2ULL : 1ULL);

	printf("mode\tpath\tbytes\tblock_size\tdirect\telapsed_nsec\tmib_per_sec\n");
	printf("%s\t%s\t%llu\t%llu\t%d\t%llu\t%.2f\n", mode_name(opts.mode),
	       opts.path, (unsigned long long)bytes,
	       (unsigned long long)opts.block_size, opts.direct,
	       (unsigned long long)elapsed,
	       fb_rate_mib_per_sec(bytes, elapsed));
	ret = 0;

out:
	free(buf);
	if (fd >= 0)
		close(fd);
	return ret;
}
