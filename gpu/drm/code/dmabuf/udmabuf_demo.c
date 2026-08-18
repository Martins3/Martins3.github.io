#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/dma-buf.h>
#include <linux/memfd.h>
#include <linux/udmabuf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

#define DEFAULT_SIZE (4U * 1024U * 1024U)

struct options {
	size_t size;
	unsigned int hold_seconds;
	bool skip_bufinfo;
};

static void print_usage(const char *prog)
{
	printf("Usage: %s [options]\n", prog);
	printf("  -s, --size BYTES        memfd / udmabuf size, default: 4194304\n");
	printf("  -t, --hold-seconds N    keep the dma-buf alive for N seconds\n");
	printf("      --skip-bufinfo      do not try to read dma_buf debugfs\n");
	printf("      --help              show this help message\n");
}

static int memfd_create_wrap(const char *name, unsigned int flags)
{
	return syscall(SYS_memfd_create, name, flags);
}

static bool parse_size(const char *arg, size_t *value)
{
	char *end = NULL;
	unsigned long long parsed = strtoull(arg, &end, 10);

	if (!arg[0] || (end && *end))
		return false;
	if (parsed == 0 || parsed > SIZE_MAX)
		return false;
	if (parsed % 4096 != 0)
		return false;

	*value = (size_t)parsed;
	return true;
}

static bool parse_uint(const char *arg, unsigned int *value)
{
	char *end = NULL;
	unsigned long parsed = strtoul(arg, &end, 10);

	if (!arg[0] || (end && *end) || parsed > UINT32_MAX)
		return false;

	*value = (unsigned int)parsed;
	return true;
}

static int parse_options(int argc, char **argv, struct options *opts)
{
	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "--help")) {
			print_usage(argv[0]);
			exit(EXIT_SUCCESS);
		}
		if (!strcmp(argv[i], "--skip-bufinfo")) {
			opts->skip_bufinfo = true;
			continue;
		}
		if (!strcmp(argv[i], "-s") || !strcmp(argv[i], "--size")) {
			if (i + 1 >= argc || !parse_size(argv[++i], &opts->size)) {
				fprintf(stderr, "Invalid size\n");
				return -1;
			}
			continue;
		}
		if (!strcmp(argv[i], "-t") || !strcmp(argv[i], "--hold-seconds")) {
			if (i + 1 >= argc ||
			    !parse_uint(argv[++i], &opts->hold_seconds)) {
				fprintf(stderr, "Invalid hold seconds\n");
				return -1;
			}
			continue;
		}

		fprintf(stderr, "Unknown option: %s\n", argv[i]);
		return -1;
	}

	return 0;
}

static int dma_buf_sync_ioctl(int dma_buf_fd, uint64_t flags)
{
	struct dma_buf_sync sync = {
		.flags = flags,
	};

	if (ioctl(dma_buf_fd, DMA_BUF_IOCTL_SYNC, &sync) < 0) {
		perror("DMA_BUF_IOCTL_SYNC");
		return -1;
	}

	return 0;
}

static int create_udmabuf(int udmabuf_fd, int memfd, size_t size)
{
	struct udmabuf_create create = {
		.memfd = (uint32_t)memfd,
		.flags = UDMABUF_FLAGS_CLOEXEC,
		.offset = 0,
		.size = size,
	};
	int dma_buf_fd;

	dma_buf_fd = ioctl(udmabuf_fd, UDMABUF_CREATE, &create);
	if (dma_buf_fd < 0)
		perror("UDMABUF_CREATE");

	return dma_buf_fd;
}

static void dump_fdinfo(int fd)
{
	char path[128];
	char line[256];
	FILE *fp;

	snprintf(path, sizeof(path), "/proc/self/fdinfo/%d", fd);
	fp = fopen(path, "r");
	if (!fp) {
		perror("fopen fdinfo");
		return;
	}

	printf("\n--- /proc/self/fdinfo/%d ---\n", fd);
	while (fgets(line, sizeof(line), fp))
		fputs(line, stdout);

	fclose(fp);
}

static void dump_matching_bufinfo(ino_t inode)
{
	FILE *fp;
	char *line = NULL;
	size_t cap = 0;
	bool matched = false;
	int header_lines = 0;

	fp = fopen("/sys/kernel/debug/dma_buf/bufinfo", "r");
	if (!fp) {
		printf("\nCould not open /sys/kernel/debug/dma_buf/bufinfo: %s\n",
		       strerror(errno));
		return;
	}

	printf("\n--- matching dma_buf/bufinfo entry ---\n");
	while (getline(&line, &cap, fp) >= 0) {
		unsigned long long parsed_ino = 0;
		int field = 0;
		char copy[512];
		char *saveptr = NULL;
		char *token = NULL;

		if (header_lines < 2) {
			fputs(line, stdout);
			++header_lines;
			continue;
		}

		if (line[0] >= '0' && line[0] <= '9') {
			snprintf(copy, sizeof(copy), "%s", line);
			for (token = strtok_r(copy, " \t\n", &saveptr); token;
			     token = strtok_r(NULL, " \t\n", &saveptr)) {
				if (field == 5) {
					parsed_ino = strtoull(token, NULL, 10);
					break;
				}
				++field;
			}
			matched = (parsed_ino == (unsigned long long)inode);
		}

		if (matched)
			fputs(line, stdout);

		if (matched && line[0] == '\n')
			break;
	}

	free(line);
	fclose(fp);
}

int main(int argc, char **argv)
{
	struct options opts = {
		.size = DEFAULT_SIZE,
	};
	unsigned char *memfd_map = NULL;
	unsigned char *dma_buf_map = NULL;
	struct stat st;
	int memfd = -1;
	int udmabuf_fd = -1;
	int dma_buf_fd = -1;
	int ret = EXIT_FAILURE;

	if (parse_options(argc, argv, &opts) < 0) {
		print_usage(argv[0]);
		return EXIT_FAILURE;
	}

	printf("udmabuf demo\n");
	printf("===========\n");
	printf("size: %zu bytes\n", opts.size);

	memfd = memfd_create_wrap("udmabuf-demo", MFD_ALLOW_SEALING | MFD_CLOEXEC);
	if (memfd < 0) {
		perror("memfd_create");
		goto out;
	}

	if (ftruncate(memfd, (off_t)opts.size) < 0) {
		perror("ftruncate");
		goto out;
	}

	if (fcntl(memfd, F_ADD_SEALS, F_SEAL_SHRINK) < 0) {
		perror("F_ADD_SEALS(F_SEAL_SHRINK)");
		goto out;
	}

	memfd_map = mmap(NULL, opts.size, PROT_READ | PROT_WRITE, MAP_SHARED,
			 memfd, 0);
	if (memfd_map == MAP_FAILED) {
		perror("mmap memfd");
		memfd_map = NULL;
		goto out;
	}

	for (size_t i = 0; i < opts.size; i += 4096)
		memfd_map[i] = (unsigned char)((i / 4096) & 0xff);
	printf("Filled memfd mapping with a page-stride pattern\n");

	udmabuf_fd = open("/dev/udmabuf", O_RDWR | O_CLOEXEC);
	if (udmabuf_fd < 0) {
		perror("open /dev/udmabuf");
		goto out;
	}

	dma_buf_fd = create_udmabuf(udmabuf_fd, memfd, opts.size);
	if (dma_buf_fd < 0)
		goto out;

	if (fstat(dma_buf_fd, &st) < 0) {
		perror("fstat dma-buf fd");
		goto out;
	}

	printf("Created udmabuf dma-buf fd\n");
	printf("  fd     : %d\n", dma_buf_fd);
	printf("  inode  : %llu\n", (unsigned long long)st.st_ino);

	dma_buf_map = mmap(NULL, opts.size, PROT_READ | PROT_WRITE, MAP_SHARED,
			   dma_buf_fd, 0);
	if (dma_buf_map == MAP_FAILED) {
		perror("mmap dma-buf fd");
		dma_buf_map = NULL;
		goto out;
	}
	printf("Mapped dma-buf fd at %p\n", dma_buf_map);

	if (dma_buf_sync_ioctl(dma_buf_fd,
			       DMA_BUF_SYNC_START | DMA_BUF_SYNC_RW) < 0)
		goto out;

	if (dma_buf_map[0] != memfd_map[0] ||
	    dma_buf_map[4096] != memfd_map[4096] ||
	    dma_buf_map[opts.size - 4096] != memfd_map[opts.size - 4096]) {
		fprintf(stderr, "dma-buf mapping does not match memfd contents\n");
		goto out_sync;
	}
	printf("dma-buf mapping sees the same initial bytes as the memfd\n");

	memset(dma_buf_map, 0xa5, 4096);
	if (memfd_map[0] != 0xa5 || memfd_map[4095] != 0xa5) {
		fprintf(stderr, "memfd mapping did not observe dma-buf writes\n");
		goto out_sync;
	}
	printf("Writing via dma-buf mapping is visible from the memfd mapping\n");

	memset(memfd_map + 4096, 0x3c, 4096);
	if (dma_buf_map[4096] != 0x3c || dma_buf_map[8191] != 0x3c) {
		fprintf(stderr, "dma-buf mapping did not observe memfd writes\n");
		goto out_sync;
	}
	printf("Writing via memfd mapping is visible from the dma-buf mapping\n");

	dump_fdinfo(dma_buf_fd);
	if (!opts.skip_bufinfo)
		dump_matching_bufinfo(st.st_ino);

	if (opts.hold_seconds > 0) {
		printf("\nHolding the udmabuf alive for %u seconds\n",
		       opts.hold_seconds);
		sleep(opts.hold_seconds);
	}

	ret = EXIT_SUCCESS;

out_sync:
	(void)dma_buf_sync_ioctl(dma_buf_fd,
				 DMA_BUF_SYNC_END | DMA_BUF_SYNC_RW);
out:
	if (dma_buf_map)
		munmap(dma_buf_map, opts.size);
	if (dma_buf_fd >= 0)
		close(dma_buf_fd);
	if (udmabuf_fd >= 0)
		close(udmabuf_fd);
	if (memfd_map)
		munmap(memfd_map, opts.size);
	if (memfd >= 0)
		close(memfd);
	return ret;
}
