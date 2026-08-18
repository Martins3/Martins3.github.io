#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <linux/memfd.h>
#include <linux/udmabuf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <xf86drm.h>

#define DRM_NODE "/dev/dri/renderD128"
#define BUFFER_SIZE (4U * 1024U * 1024U)

static int memfd_create_wrap(const char *name, unsigned int flags)
{
	return syscall(SYS_memfd_create, name, flags);
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

static int import_into_drm(int drm_fd, int dma_buf_fd, uint32_t *handle)
{
	struct drm_prime_handle prime = {
		.fd = dma_buf_fd,
	};

	if (drmIoctl(drm_fd, DRM_IOCTL_PRIME_FD_TO_HANDLE, &prime) < 0) {
		perror("DRM_IOCTL_PRIME_FD_TO_HANDLE");
		return -1;
	}

	*handle = prime.handle;
	return 0;
}

static void close_gem_handle(int drm_fd, uint32_t handle)
{
	struct drm_gem_close close_arg = {
		.handle = handle,
	};

	if (drmIoctl(drm_fd, DRM_IOCTL_GEM_CLOSE, &close_arg) < 0)
		perror("DRM_IOCTL_GEM_CLOSE");
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
	int header_lines = 0;
	int print_current_block = 0;

	fp = fopen("/sys/kernel/debug/dma_buf/bufinfo", "r");
	if (!fp) {
		printf("\nCould not open /sys/kernel/debug/dma_buf/bufinfo: %s\n",
		       strerror(errno));
		return;
	}

	printf("\n--- matching dma_buf/bufinfo entry ---\n");
	while (getline(&line, &cap, fp) >= 0) {
		if (header_lines < 2) {
			fputs(line, stdout);
			++header_lines;
			continue;
		}

		if (line[0] >= '0' && line[0] <= '9') {
			unsigned long long parsed_ino = 0;
			int field = 0;
			char copy[512];
			char *saveptr = NULL;
			char *token = NULL;

			snprintf(copy, sizeof(copy), "%s", line);
			for (token = strtok_r(copy, " \t\n", &saveptr); token;
			     token = strtok_r(NULL, " \t\n", &saveptr)) {
				if (field == 5) {
					parsed_ino = strtoull(token, NULL, 10);
					break;
				}
				++field;
			}

			print_current_block =
				parsed_ino == (unsigned long long)inode;
		}

		if (print_current_block)
			fputs(line, stdout);

		if (print_current_block && line[0] == '\n')
			break;
	}

	free(line);
	fclose(fp);
}

int main(void)
{
	unsigned char *memfd_map = NULL;
	unsigned char *dma_buf_map = NULL;
	struct stat st;
	uint32_t gem_handle = 0;
	int memfd = -1;
	int udmabuf_fd = -1;
	int dma_buf_fd = -1;
	int drm_fd = -1;
	int ret = EXIT_FAILURE;

	printf("udmabuf -> DRM PRIME import demo\n");
	printf("================================\n");
	printf("DRM node    : %s\n", DRM_NODE);
	printf("buffer size : %u bytes\n", BUFFER_SIZE);

	memfd = memfd_create_wrap("udmabuf-import-demo",
				  MFD_ALLOW_SEALING | MFD_CLOEXEC);
	if (memfd < 0) {
		perror("memfd_create");
		goto out;
	}

	if (ftruncate(memfd, BUFFER_SIZE) < 0) {
		perror("ftruncate");
		goto out;
	}

	if (fcntl(memfd, F_ADD_SEALS, F_SEAL_SHRINK) < 0) {
		perror("F_ADD_SEALS(F_SEAL_SHRINK)");
		goto out;
	}

	memfd_map = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
			 memfd, 0);
	if (memfd_map == MAP_FAILED) {
		perror("mmap memfd");
		memfd_map = NULL;
		goto out;
	}

	for (size_t i = 0; i < BUFFER_SIZE; i += 4096)
		memfd_map[i] = (unsigned char)((i / 4096) & 0xff);
	printf("Filled memfd with a page-stride pattern\n");

	udmabuf_fd = open("/dev/udmabuf", O_RDWR | O_CLOEXEC);
	if (udmabuf_fd < 0) {
		perror("open /dev/udmabuf");
		goto out;
	}

	dma_buf_fd = create_udmabuf(udmabuf_fd, memfd, BUFFER_SIZE);
	if (dma_buf_fd < 0)
		goto out;

	if (fstat(dma_buf_fd, &st) < 0) {
		perror("fstat dma-buf fd");
		goto out;
	}

	printf("Created udmabuf dma-buf fd\n");
	printf("  fd    : %d\n", dma_buf_fd);
	printf("  inode : %llu\n", (unsigned long long)st.st_ino);

	dma_buf_map = mmap(NULL, BUFFER_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
			   dma_buf_fd, 0);
	if (dma_buf_map == MAP_FAILED) {
		perror("mmap dma-buf fd");
		dma_buf_map = NULL;
		goto out;
	}

	if (dma_buf_map[0] != memfd_map[0] ||
	    dma_buf_map[4096] != memfd_map[4096]) {
		fprintf(stderr, "dma-buf mapping does not match memfd contents\n");
		goto out;
	}
	printf("dma-buf mapping sees the same pages as the memfd\n");

	printf("\nBefore PRIME import\n");
	dump_fdinfo(dma_buf_fd);
	dump_matching_bufinfo(st.st_ino);

	drm_fd = open(DRM_NODE, O_RDWR | O_CLOEXEC);
	if (drm_fd < 0) {
		perror("open DRM node");
		goto out;
	}

	if (import_into_drm(drm_fd, dma_buf_fd, &gem_handle) < 0)
		goto out;

	printf("\nImported dma-buf into DRM\n");
	printf("  GEM handle : %u\n", gem_handle);
	dump_matching_bufinfo(st.st_ino);

	close_gem_handle(drm_fd, gem_handle);
	gem_handle = 0;

	printf("\nAfter DRM_IOCTL_GEM_CLOSE\n");
	printf("Attachment may still be cached by the DRM file\n");
	dump_matching_bufinfo(st.st_ino);

	close(drm_fd);
	drm_fd = -1;

	printf("\nAfter close(drm_fd)\n");
	dump_matching_bufinfo(st.st_ino);

	ret = EXIT_SUCCESS;

out:
	if (gem_handle)
		close_gem_handle(drm_fd, gem_handle);
	if (dma_buf_map)
		munmap(dma_buf_map, BUFFER_SIZE);
	if (dma_buf_fd >= 0)
		close(dma_buf_fd);
	if (udmabuf_fd >= 0)
		close(udmabuf_fd);
	if (memfd_map)
		munmap(memfd_map, BUFFER_SIZE);
	if (memfd >= 0)
		close(memfd);
	if (drm_fd >= 0)
		close(drm_fd);
	return ret;
}
