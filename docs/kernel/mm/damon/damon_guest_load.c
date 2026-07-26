// SPDX-License-Identifier: GPL-2.0
/*
 * Guest-side memory workload for DAMON locality experiments.
 *
 * Build in the VM:
 *   gcc -O2 -Wall -Wextra -o damon_guest_load.guest.out damon_guest_load.c
 *
 * Examples:
 *   ./damon_guest_load.guest.out --mode seq-hotset --total-mb 2048 --hot-mb 256 --seconds 30
 *   ./damon_guest_load.guest.out --mode random-hotset --total-mb 2048 --hot-mb 256 --seconds 30
 *   ./damon_guest_load.guest.out --mode random-full --total-mb 2048 --seconds 30
 */

#define _GNU_SOURCE

#include <errno.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define PAGE_SZ 4096UL
#define CACHELINE 64UL

enum mode {
	MODE_SEQ_HOTSET,
	MODE_RANDOM_HOTSET,
	MODE_RANDOM_FULL,
};

struct options {
	enum mode mode;
	size_t total_mb;
	size_t hot_mb;
	unsigned int seconds;
};

static volatile uint64_t sink;

static uint64_t nsec_now(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts))
		return 0;
	return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static uint64_t xorshift64(uint64_t *state)
{
	uint64_t x = *state;

	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	*state = x;
	return x;
}

static size_t parse_size_mb(const char *name, const char *value)
{
	char *end;
	unsigned long long parsed;

	errno = 0;
	parsed = strtoull(value, &end, 0);
	if (errno || end == value || *end != '\0' || parsed == 0) {
		fprintf(stderr, "invalid %s: %s\n", name, value);
		exit(EXIT_FAILURE);
	}
	return (size_t)parsed;
}

static enum mode parse_mode(const char *value)
{
	if (!strcmp(value, "seq-hotset"))
		return MODE_SEQ_HOTSET;
	if (!strcmp(value, "random-hotset"))
		return MODE_RANDOM_HOTSET;
	if (!strcmp(value, "random-full"))
		return MODE_RANDOM_FULL;
	fprintf(stderr, "invalid mode: %s\n", value);
	exit(EXIT_FAILURE);
}

static const char *mode_name(enum mode mode)
{
	switch (mode) {
	case MODE_SEQ_HOTSET:
		return "seq-hotset";
	case MODE_RANDOM_HOTSET:
		return "random-hotset";
	case MODE_RANDOM_FULL:
		return "random-full";
	}
	return "unknown";
}

static void usage(const char *argv0)
{
	fprintf(stderr,
		"usage: %s --mode MODE [--total-mb N] [--hot-mb N] [--seconds N]\n"
		"\n"
		"modes: seq-hotset, random-hotset, random-full\n"
		"defaults: --total-mb 2048 --hot-mb 256 --seconds 30\n",
		argv0);
}

static void parse_args(int argc, char **argv, struct options *opts)
{
	*opts = (struct options) {
		.mode = MODE_SEQ_HOTSET,
		.total_mb = 2048,
		.hot_mb = 256,
		.seconds = 30,
	};

	for (int i = 1; i < argc; i++) {
		const char *arg = argv[i];
		const char *val = i + 1 < argc ? argv[i + 1] : NULL;

		if (!strcmp(arg, "-h") || !strcmp(arg, "--help")) {
			usage(argv[0]);
			exit(EXIT_SUCCESS);
		}
		if (!val) {
			fprintf(stderr, "missing value for %s\n", arg);
			exit(EXIT_FAILURE);
		}
		if (!strcmp(arg, "--mode")) {
			opts->mode = parse_mode(val);
			i++;
		} else if (!strcmp(arg, "--total-mb")) {
			opts->total_mb = parse_size_mb(arg, val);
			i++;
		} else if (!strcmp(arg, "--hot-mb")) {
			opts->hot_mb = parse_size_mb(arg, val);
			i++;
		} else if (!strcmp(arg, "--seconds")) {
			opts->seconds = (unsigned int)parse_size_mb(arg, val);
			i++;
		} else {
			fprintf(stderr, "unknown option: %s\n", arg);
			exit(EXIT_FAILURE);
		}
	}

	if (opts->hot_mb > opts->total_mb)
		opts->hot_mb = opts->total_mb;
}

static void prefault(uint8_t *mem, size_t bytes)
{
	for (size_t off = 0; off < bytes; off += PAGE_SZ)
		mem[off] = (uint8_t)off;
}

static uint64_t run_seq(uint8_t *mem, size_t hot_bytes, uint64_t end_ns)
{
	uint64_t ops = 0;

	while (nsec_now() < end_ns) {
		for (size_t off = 0; off < hot_bytes; off += CACHELINE) {
			mem[off]++;
			sink += mem[off];
			ops++;
		}
	}
	return ops;
}

static uint64_t run_random(uint8_t *mem, size_t bytes, uint64_t end_ns)
{
	uint64_t seed = 0x123456789abcdefULL ^ (uint64_t)getpid();
	size_t pages = bytes / PAGE_SZ;
	uint64_t ops = 0;

	if (!pages)
		pages = 1;

	while (nsec_now() < end_ns) {
		size_t page = xorshift64(&seed) % pages;
		size_t off = page * PAGE_SZ + ((xorshift64(&seed) % (PAGE_SZ / CACHELINE)) * CACHELINE);

		mem[off]++;
		sink += mem[off];
		ops++;
	}
	return ops;
}

int main(int argc, char **argv)
{
	struct options opts;
	size_t total_bytes;
	size_t hot_bytes;
	uint8_t *mem;
	uint64_t start_ns;
	uint64_t end_ns;
	uint64_t ops;

	parse_args(argc, argv, &opts);
	total_bytes = opts.total_mb * 1024UL * 1024UL;
	hot_bytes = opts.hot_mb * 1024UL * 1024UL;

	mem = mmap(NULL, total_bytes, PROT_READ | PROT_WRITE,
		   MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (mem == MAP_FAILED) {
		perror("mmap");
		return EXIT_FAILURE;
	}

	printf("mode=%s total=%zu MiB hot=%zu MiB seconds=%u pid=%d\n",
	       mode_name(opts.mode), opts.total_mb, opts.hot_mb,
	       opts.seconds, getpid());
	fflush(stdout);

	prefault(mem, total_bytes);
	if (madvise(mem, total_bytes, MADV_NOHUGEPAGE))
		perror("madvise(MADV_NOHUGEPAGE)");

	start_ns = nsec_now();
	end_ns = start_ns + (uint64_t)opts.seconds * 1000000000ULL;
	switch (opts.mode) {
	case MODE_SEQ_HOTSET:
		ops = run_seq(mem, hot_bytes, end_ns);
		break;
	case MODE_RANDOM_HOTSET:
		ops = run_random(mem, hot_bytes, end_ns);
		break;
	case MODE_RANDOM_FULL:
		ops = run_random(mem, total_bytes, end_ns);
		break;
	default:
		ops = 0;
		break;
	}

	printf("done mode=%s ops=%" PRIu64 " sink=%" PRIu64 "\n",
	       mode_name(opts.mode), ops, sink);
	munmap(mem, total_bytes);
	return EXIT_SUCCESS;
}
