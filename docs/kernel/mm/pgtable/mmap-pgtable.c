#define _GNU_SOURCE

#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <time.h>
#include <unistd.h>

#ifndef MADV_POPULATE_READ
#define MADV_POPULATE_READ 22
#endif

#define DEFAULT_SIZE (1ULL << 40)
#define POPULATE_CHUNK (1ULL << 30)

struct snapshot {
	uint64_t vm_size_kib;
	uint64_t vm_rss_kib;
	uint64_t rss_anon_kib;
	uint64_t vm_pte_kib;
	uint64_t page_tables_kib;
	long minor_faults;
};

static void usage(const char *prog)
{
	fprintf(stderr,
		"Usage: %s [SIZE] [--hold SECONDS]\n"
		"  SIZE defaults to 1T; suffixes K, M, G and T are binary.\n"
		"  --hold 0 waits for Enter after populating the page tables.\n",
		prog);
}

static uint64_t parse_size(const char *arg)
{
	char *end;
	uint64_t multiplier = 1;
	errno = 0;
	unsigned long long value = strtoull(arg, &end, 0);

	if (errno || end == arg)
		goto invalid;
	if (*end != '\0') {
		switch (*end++) {
		case 'k':
		case 'K':
			multiplier = 1ULL << 10;
			break;
		case 'm':
		case 'M':
			multiplier = 1ULL << 20;
			break;
		case 'g':
		case 'G':
			multiplier = 1ULL << 30;
			break;
		case 't':
		case 'T':
			multiplier = 1ULL << 40;
			break;
		default:
			goto invalid;
		}
		if (*end == 'i' || *end == 'I')
			end++;
		if (*end == 'b' || *end == 'B')
			end++;
		if (*end != '\0')
			goto invalid;
	}
	if (!value || value > UINT64_MAX / multiplier)
		goto invalid;
	return value * multiplier;

invalid:
	fprintf(stderr, "Invalid size: %s\n", arg);
	exit(EXIT_FAILURE);
}

static uint64_t read_kib_field(const char *path, const char *field)
{
	FILE *fp = fopen(path, "re");
	char *line = NULL;
	size_t capacity = 0;
	uint64_t value = 0;

	if (!fp)
		return 0;
	while (getline(&line, &capacity, fp) >= 0) {
		if (sscanf(line, "%*[^:]: %" SCNu64 " kB", &value) == 1 &&
		    strncmp(line, field, strlen(field)) == 0 &&
		    line[strlen(field)] == ':')
			break;
		value = 0;
	}
	free(line);
	fclose(fp);
	return value;
}

static struct snapshot take_snapshot(void)
{
	struct rusage usage;
	struct snapshot s = {
		.vm_size_kib = read_kib_field("/proc/self/status", "VmSize"),
		.vm_rss_kib = read_kib_field("/proc/self/status", "VmRSS"),
		.rss_anon_kib = read_kib_field("/proc/self/status", "RssAnon"),
		.vm_pte_kib = read_kib_field("/proc/self/status", "VmPTE"),
		.page_tables_kib = read_kib_field("/proc/meminfo", "PageTables"),
	};

	if (getrusage(RUSAGE_SELF, &usage) == 0)
		s.minor_faults = usage.ru_minflt;
	return s;
}

static void print_snapshot(const char *name, const struct snapshot *s)
{
	printf("%-6s VmSize=%10" PRIu64 " KiB  VmRSS=%8" PRIu64
	       " KiB  RssAnon=%8" PRIu64 " KiB  VmPTE=%8" PRIu64
	       " KiB  system_PageTables=%8" PRIu64 " KiB  minflt=%ld\n",
	       name, s->vm_size_kib, s->vm_rss_kib, s->rss_anon_kib,
	       s->vm_pte_kib, s->page_tables_kib, s->minor_faults);
}

static void print_delta(const struct snapshot *before,
			const struct snapshot *after)
{
	printf("delta  VmRSS=%+8" PRId64 " KiB  RssAnon=%+8" PRId64
	       " KiB  VmPTE=%+8" PRId64 " KiB  system_PageTables=%+8"
	       PRId64 " KiB  minflt=%+ld\n",
	       (int64_t)after->vm_rss_kib - (int64_t)before->vm_rss_kib,
	       (int64_t)after->rss_anon_kib - (int64_t)before->rss_anon_kib,
	       (int64_t)after->vm_pte_kib - (int64_t)before->vm_pte_kib,
	       (int64_t)after->page_tables_kib -
		       (int64_t)before->page_tables_kib,
	       after->minor_faults - before->minor_faults);
}

static double elapsed_seconds(const struct timespec *start,
			      const struct timespec *end)
{
	return end->tv_sec - start->tv_sec +
	       (end->tv_nsec - start->tv_nsec) / 1000000000.0;
}

int main(int argc, char **argv)
{
	uint64_t requested_size = DEFAULT_SIZE;
	unsigned int hold_seconds = UINT_MAX;
	long page_size = sysconf(_SC_PAGESIZE);
	bool size_seen = false;

	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--hold")) {
			char *end;
			unsigned long value;

			if (++i == argc) {
				usage(argv[0]);
				return EXIT_FAILURE;
			}
			errno = 0;
			value = strtoul(argv[i], &end, 10);
			if (errno || *end != '\0' || value > UINT_MAX) {
				usage(argv[0]);
				return EXIT_FAILURE;
			}
			hold_seconds = (unsigned int)value;
		} else if (!size_seen) {
			requested_size = parse_size(argv[i]);
			size_seen = true;
		} else {
			usage(argv[0]);
			return EXIT_FAILURE;
		}
	}

	if (page_size <= 0) {
		perror("sysconf(_SC_PAGESIZE)");
		return EXIT_FAILURE;
	}
	if (requested_size > SIZE_MAX - (uint64_t)page_size + 1) {
		fprintf(stderr, "Mapping size does not fit in size_t\n");
		return EXIT_FAILURE;
	}
	size_t size = (size_t)((requested_size + page_size - 1) &
			       ~((uint64_t)page_size - 1));

	struct snapshot before = take_snapshot();
	void *mapping = mmap(NULL, size, PROT_READ,
			     MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
	if (mapping == MAP_FAILED) {
		perror("mmap");
		return EXIT_FAILURE;
	}
	if (madvise(mapping, size, MADV_NOHUGEPAGE) == -1) {
		perror("madvise(MADV_NOHUGEPAGE)");
		munmap(mapping, size);
		return EXIT_FAILURE;
	}

	printf("pid=%ld address=%p size=%zu bytes (%.3f GiB) page_size=%ld\n",
	       (long)getpid(), mapping, size, size / (double)(1ULL << 30),
	       page_size);
	print_snapshot("before", &before);

	struct timespec start, end;
	clock_gettime(CLOCK_MONOTONIC, &start);
	unsigned int last_percent = 0;
	for (size_t offset = 0; offset < size;) {
		size_t length = size - offset;
		if (length > POPULATE_CHUNK)
			length = POPULATE_CHUNK;
		if (madvise((char *)mapping + offset, length,
			    MADV_POPULATE_READ) == -1) {
			fprintf(stderr,
				"madvise(MADV_POPULATE_READ) failed at offset "
				"%zu: %s\n",
				offset, strerror(errno));
			munmap(mapping, size);
			return EXIT_FAILURE;
		}
		offset += length;
		unsigned int percent = (unsigned int)(((__uint128_t)offset * 100) /
						      size);
		if (percent >= last_percent + 5 || offset == size) {
			fprintf(stderr, "\rpopulating: %3u%%", percent);
			fflush(stderr);
			last_percent = percent;
		}
	}
	fputc('\n', stderr);
	clock_gettime(CLOCK_MONOTONIC, &end);

	struct snapshot after = take_snapshot();
	print_snapshot("after", &after);
	print_delta(&before, &after);
	printf("elapsed=%.3f seconds\n", elapsed_seconds(&start, &end));
	printf("The mapping is read-only: faults map the shared zero page; "
	       "VmPTE is the primary per-mm page-table counter.\n");

	if (hold_seconds != UINT_MAX) {
		if (hold_seconds == 0) {
			printf("Press Enter to unmap and exit...\n");
			(void)getchar();
		} else {
			printf("Holding the mapping for %u seconds...\n",
			       hold_seconds);
			sleep(hold_seconds);
		}
	}

	if (munmap(mapping, size) == -1) {
		perror("munmap");
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
