#define _GNU_SOURCE

#include <errno.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#if defined(__x86_64__) || defined(__i386__)
#include <x86intrin.h>
#endif

static void *(*volatile copy_fn)(void *, const void *, size_t) = memcpy;

static uint64_t read_counter(void)
{
#if defined(__x86_64__) || defined(__i386__)
	unsigned int aux;

	_mm_lfence();
	return __rdtscp(&aux);
#elif defined(__aarch64__)
	uint64_t v;

	asm volatile("isb; mrs %0, cntvct_el0" : "=r"(v));
	return v;
#else
	return 0;
#endif
}

static uint64_t nsec_now(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC_RAW, &ts) != 0) {
		perror("clock_gettime");
		exit(EXIT_FAILURE);
	}

	return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static size_t parse_size(const char *s, const char *name)
{
	char *end = NULL;
	unsigned long long v;

	errno = 0;
	v = strtoull(s, &end, 0);
	if (errno || end == s || *end != '\0' || v == 0) {
		fprintf(stderr, "invalid %s: %s\n", name, s);
		exit(EXIT_FAILURE);
	}

	return (size_t)v;
}

static void usage(const char *argv0)
{
	fprintf(stderr, "usage: %s [copy_size] [pages] [iterations]\n", argv0);
	fprintf(stderr, "default: one system page, 1 page, 10000000 iterations\n");
}

int main(int argc, char **argv)
{
	const size_t page_size = (size_t)sysconf(_SC_PAGESIZE);
	size_t copy_size = page_size;
	size_t pages = 1;
	uint64_t iterations = 10000000ULL;
	size_t alloc_size;
	unsigned char *src;
	unsigned char *dst;
	uint64_t start_ns;
	uint64_t end_ns;
	uint64_t start_counter;
	uint64_t end_counter;
	uint64_t elapsed_ns;
	uint64_t elapsed_counter;
	volatile uint64_t checksum = 0;

	if (argc > 4) {
		usage(argv[0]);
		return EXIT_FAILURE;
	}
	if (argc > 1)
		copy_size = parse_size(argv[1], "copy_size");
	if (argc > 2)
		pages = parse_size(argv[2], "pages");
	if (argc > 3)
		iterations = parse_size(argv[3], "iterations");

	alloc_size = copy_size * pages;
	if (pages != 0 && alloc_size / pages != copy_size) {
		fprintf(stderr, "allocation size overflow\n");
		return EXIT_FAILURE;
	}

	if (posix_memalign((void **)&src, page_size, alloc_size) != 0 ||
	    posix_memalign((void **)&dst, page_size, alloc_size) != 0) {
		perror("posix_memalign");
		return EXIT_FAILURE;
	}

	for (size_t i = 0; i < alloc_size; i++)
		src[i] = (unsigned char)(i * 131u + 7u);
	memset(dst, 0, alloc_size);

	if (pages == 1) {
		for (uint64_t i = 0; i < 10000; i++)
			copy_fn(dst, src, copy_size);
	} else {
		for (uint64_t i = 0, offset = 0; i < 10000; i++) {
			copy_fn(dst + offset, src + offset, copy_size);
			offset += copy_size;
			if (offset == alloc_size)
				offset = 0;
		}
	}

	start_counter = read_counter();
	start_ns = nsec_now();

	if (pages == 1) {
		for (uint64_t i = 0; i < iterations; i++)
			copy_fn(dst, src, copy_size);
	} else {
		for (uint64_t i = 0, offset = 0; i < iterations; i++) {
			copy_fn(dst + offset, src + offset, copy_size);
			offset += copy_size;
			if (offset == alloc_size)
				offset = 0;
		}
	}

	end_ns = nsec_now();
	end_counter = read_counter();

	for (size_t i = 0; i < pages; i++)
		checksum += dst[i * copy_size] + dst[i * copy_size + copy_size - 1];

	elapsed_ns = end_ns - start_ns;
	elapsed_counter = end_counter - start_counter;

	printf("copy_size:  %zu bytes\n", copy_size);
	printf("pages:      %zu\n", pages);
	printf("iterations: %" PRIu64 "\n", iterations);
	printf("total:      %.3f GiB\n",
	       (double)copy_size * (double)iterations / 1024.0 / 1024.0 / 1024.0);
	printf("elapsed:    %.6f s\n", (double)elapsed_ns / 1000000000.0);
	printf("latency:    %.2f ns/copy\n", (double)elapsed_ns / (double)iterations);
	printf("bandwidth:  %.2f GiB/s\n",
	       ((double)copy_size * (double)iterations / 1024.0 / 1024.0 / 1024.0) /
		       ((double)elapsed_ns / 1000000000.0));
	if (elapsed_counter != 0)
		printf("counter:    %.2f ticks/copy\n",
		       (double)elapsed_counter / (double)iterations);
	printf("checksum:   %" PRIu64 "\n", checksum);

	free(src);
	free(dst);
	return EXIT_SUCCESS;
}
