#define _GNU_SOURCE
#include <ctype.h>
#include <drm/drm_fourcc.h>
#include <drm/virtgpu_drm.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <glob.h>
#include <inttypes.h>
#include <linux/dma-buf.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#ifndef VIRTGPU_PARAM_NUM_SCANOUTS
#define VIRTGPU_PARAM_NUM_SCANOUTS 2
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

struct options {
	const char *device_path;
	uint32_t width;
	uint32_t height;
	uint32_t bpp;
	unsigned int hold_seconds;
	bool list_only;
	bool skip_bufinfo;
};

struct drm_node_info {
	char path[128];
	char driver[128];
	char desc[256];
	char date[64];
	uint64_t dumb_buffer_cap;
};

struct test_buffer {
	uint32_t handle;
	uint32_t pitch;
	uint32_t width;
	uint32_t height;
	uint64_t size;
	uint32_t format;
	void *map_addr;
	int dma_buf_fd;
	ino_t dma_buf_inode;
};

static void print_usage(const char *prog)
{
	printf("Usage: %s [options]\n", prog);
	printf("  -d, --device PATH         DRM device node, default: auto-pick\n");
	printf("  -w, --width N            Buffer width in pixels, default: 512\n");
	printf("  -h, --height N           Buffer height in pixels, default: 512\n");
	printf("  -b, --bpp N              Bits per pixel, default: 32\n");
	printf("  -s, --hold-seconds N     Keep the buffer alive for N seconds\n");
	printf("  -l, --list               List DRM card nodes and exit\n");
	printf("      --skip-bufinfo       Do not read /sys/kernel/debug/dma_buf/bufinfo\n");
	printf("      --help               Show this help message\n");
}

static bool parse_u32(const char *arg, uint32_t *value)
{
	char *end = NULL;
	unsigned long parsed = strtoul(arg, &end, 10);

	if (!arg[0] || (end && *end) || parsed > UINT32_MAX)
		return false;

	*value = (uint32_t)parsed;
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
	static const struct option long_options[] = {
		{ "device", required_argument, NULL, 'd' },
		{ "width", required_argument, NULL, 'w' },
		{ "height", required_argument, NULL, 'h' },
		{ "bpp", required_argument, NULL, 'b' },
		{ "hold-seconds", required_argument, NULL, 's' },
		{ "list", no_argument, NULL, 'l' },
		{ "skip-bufinfo", no_argument, NULL, 1 },
		{ "help", no_argument, NULL, 2 },
		{ 0, 0, 0, 0 },
	};
	int opt;

	while ((opt = getopt_long(argc, argv, "d:w:h:b:s:l", long_options,
				  NULL)) != -1) {
		switch (opt) {
		case 'd':
			opts->device_path = optarg;
			break;
		case 'w':
			if (!parse_u32(optarg, &opts->width)) {
				fprintf(stderr, "Invalid width: %s\n", optarg);
				return -1;
			}
			break;
		case 'h':
			if (!parse_u32(optarg, &opts->height)) {
				fprintf(stderr, "Invalid height: %s\n", optarg);
				return -1;
			}
			break;
		case 'b':
			if (!parse_u32(optarg, &opts->bpp)) {
				fprintf(stderr, "Invalid bpp: %s\n", optarg);
				return -1;
			}
			break;
		case 's':
			if (!parse_uint(optarg, &opts->hold_seconds)) {
				fprintf(stderr, "Invalid hold seconds: %s\n",
					optarg);
				return -1;
			}
			break;
		case 'l':
			opts->list_only = true;
			break;
		case 1:
			opts->skip_bufinfo = true;
			break;
		case 2:
			print_usage(argv[0]);
			exit(EXIT_SUCCESS);
		default:
			print_usage(argv[0]);
			return -1;
		}
	}

	return 0;
}

static int open_drm_node(const char *path)
{
	return open(path, O_RDWR | O_CLOEXEC);
}

static int fill_node_info(const char *path, struct drm_node_info *info)
{
	drmVersion *version = NULL;
	int drm_fd;

	memset(info, 0, sizeof(*info));
	snprintf(info->path, sizeof(info->path), "%s", path);

	drm_fd = open_drm_node(path);
	if (drm_fd < 0)
		return -1;

	version = drmGetVersion(drm_fd);
	if (version) {
		snprintf(info->driver, sizeof(info->driver), "%.*s",
			 version->name_len, version->name);
		snprintf(info->desc, sizeof(info->desc), "%.*s",
			 version->desc_len, version->desc);
		snprintf(info->date, sizeof(info->date), "%.*s",
			 version->date_len, version->date);
		drmFreeVersion(version);
	} else {
		snprintf(info->driver, sizeof(info->driver), "<unknown>");
	}

	if (drmGetCap(drm_fd, DRM_CAP_DUMB_BUFFER, &info->dumb_buffer_cap)) {
		info->dumb_buffer_cap = 0;
	}

	close(drm_fd);
	return 0;
}

static int collect_drm_nodes(struct drm_node_info *infos, size_t max_infos)
{
	glob_t glob_result = { 0 };
	size_t i;
	int count = 0;

	if (glob("/dev/dri/card*", 0, NULL, &glob_result) != 0)
		return 0;

	for (i = 0; i < glob_result.gl_pathc && count < (int)max_infos; ++i) {
		if (fill_node_info(glob_result.gl_pathv[i], &infos[count]) == 0)
			++count;
	}

	globfree(&glob_result);
	return count;
}

static void print_node_info(const struct drm_node_info *info)
{
	printf("%s\n", info->path);
	printf("  driver      : %s\n",
	       info->driver[0] ? info->driver : "<unknown>");
	printf("  desc        : %s\n", info->desc[0] ? info->desc : "<none>");
	printf("  date        : %s\n", info->date[0] ? info->date : "<none>");
	printf("  dumb buffer : %s\n",
	       info->dumb_buffer_cap ? "YES" : "NO");
}

static const struct drm_node_info *pick_default_node(
	const struct drm_node_info *infos, int count)
{
	int i;

	for (i = 0; i < count; ++i) {
		if (!infos[i].dumb_buffer_cap)
			continue;
		if (strcmp(infos[i].driver, "vkms") != 0)
			return &infos[i];
	}

	for (i = 0; i < count; ++i) {
		if (infos[i].dumb_buffer_cap)
			return &infos[i];
	}

	return NULL;
}

static void dump_fdinfo(int fd)
{
	char path[128];
	char line[256];
	FILE *fp;

	snprintf(path, sizeof(path), "/proc/self/fdinfo/%d", fd);
	fp = fopen(path, "r");
	if (!fp) {
		fprintf(stderr, "Could not open %s: %s\n", path, strerror(errno));
		return;
	}

	printf("\n--- /proc/self/fdinfo/%d ---\n", fd);
	while (fgets(line, sizeof(line), fp))
		fputs(line, stdout);

	fclose(fp);
}

static bool parse_bufinfo_inode(const char *line, unsigned long long *inode)
{
	char copy[512];
	char *saveptr = NULL;
	char *token = NULL;
	int field = 0;

	if (!isdigit((unsigned char)line[0]))
		return false;

	snprintf(copy, sizeof(copy), "%s", line);
	for (token = strtok_r(copy, " \t\n", &saveptr); token;
	     token = strtok_r(NULL, " \t\n", &saveptr)) {
		if (field == 5) {
			*inode = strtoull(token, NULL, 10);
			return true;
		}
		++field;
	}

	return false;
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
		printf("\n--- dma_buf/bufinfo ---\n");
		printf("Could not open /sys/kernel/debug/dma_buf/bufinfo: %s\n",
		       strerror(errno));
		printf("Hint: run as root or use `echo a | sudo -S cat /sys/kernel/debug/dma_buf/bufinfo`.\n");
		return;
	}

	printf("\n--- dma_buf/bufinfo note ---\n");
	printf("`exp_name` is the dma-buf exporter name, often just `drm`.\n");
	printf("It does not tell you which DRM driver created the BO.\n");
	printf("Use the dma-buf inode to correlate fdinfo with bufinfo.\n");

	while (getline(&line, &cap, fp) >= 0) {
		unsigned long long parsed_inode = 0;

		if (header_lines < 2) {
			fputs(line, stdout);
			++header_lines;
			continue;
		}

		if (parse_bufinfo_inode(line, &parsed_inode)) {
			matched = (parsed_inode == (unsigned long long)inode);
		}

		if (matched)
			fputs(line, stdout);

		if (matched && line[0] == '\n')
			break;
	}

	if (!matched) {
		printf("No matching bufinfo entry for dma-buf inode %llu.\n",
		       (unsigned long long)inode);
	}

	free(line);
	fclose(fp);
}

static int create_test_buffer(int drm_fd, struct test_buffer *buf,
			      const struct options *opts)
{
	struct drm_mode_create_dumb create_arg = { 0 };

	create_arg.width = opts->width;
	create_arg.height = opts->height;
	create_arg.bpp = opts->bpp;

	if (drmIoctl(drm_fd, DRM_IOCTL_MODE_CREATE_DUMB, &create_arg) < 0) {
		perror("Failed to create dumb buffer");
		return -1;
	}

	buf->handle = create_arg.handle;
	buf->pitch = create_arg.pitch;
	buf->width = create_arg.width;
	buf->height = create_arg.height;
	buf->size = create_arg.size;
	buf->format = DRM_FORMAT_XRGB8888;

	printf("Created dumb buffer\n");
	printf("  handle : %u\n", buf->handle);
	printf("  size   : %" PRIu64 " bytes\n", buf->size);
	printf("  pitch  : %u bytes\n", buf->pitch);
	printf("  w/h    : %ux%u\n", buf->width, buf->height);
	printf("  bpp    : %u\n", opts->bpp);

	return 0;
}

static int export_buffer_as_dma_buf(int drm_fd, struct test_buffer *buf)
{
	struct drm_prime_handle prime_arg = { 0 };
	struct stat st;

	prime_arg.handle = buf->handle;
	prime_arg.flags = DRM_CLOEXEC | DRM_RDWR;

	if (drmIoctl(drm_fd, DRM_IOCTL_PRIME_HANDLE_TO_FD, &prime_arg) < 0) {
		perror("Failed to export buffer as DMA-BUF");
		return -1;
	}

	buf->dma_buf_fd = prime_arg.fd;
	if (fstat(buf->dma_buf_fd, &st) < 0) {
		perror("Failed to fstat DMA-BUF fd");
		return -1;
	}

	buf->dma_buf_inode = st.st_ino;
	printf("Exported as dma-buf\n");
	printf("  fd     : %d\n", buf->dma_buf_fd);
	printf("  inode  : %llu (0x%llx)\n",
	       (unsigned long long)buf->dma_buf_inode,
	       (unsigned long long)buf->dma_buf_inode);

	return 0;
}

static int map_buffer_for_cpu(int drm_fd, struct test_buffer *buf)
{
	struct drm_mode_map_dumb map_arg = { 0 };

	map_arg.handle = buf->handle;
	if (drmIoctl(drm_fd, DRM_IOCTL_MODE_MAP_DUMB, &map_arg) < 0) {
		perror("Failed to map dumb buffer");
		return -1;
	}

	buf->map_addr = mmap(NULL, buf->size, PROT_READ | PROT_WRITE, MAP_SHARED,
			     drm_fd, map_arg.offset);
	if (buf->map_addr == MAP_FAILED) {
		perror("Failed to mmap dumb buffer");
		buf->map_addr = NULL;
		return -1;
	}

	printf("Mapped dumb buffer at %p\n", buf->map_addr);
	return 0;
}

static int test_dma_buf_sync(int dma_buf_fd)
{
	struct dma_buf_sync sync_start = { 0 };
	struct dma_buf_sync sync_end = { 0 };

	sync_start.flags = DMA_BUF_SYNC_WRITE | DMA_BUF_SYNC_START;
	if (ioctl(dma_buf_fd, DMA_BUF_IOCTL_SYNC, &sync_start) < 0) {
		perror("Failed DMA_BUF_SYNC_START");
		return -1;
	}

	sync_end.flags = DMA_BUF_SYNC_WRITE | DMA_BUF_SYNC_END;
	if (ioctl(dma_buf_fd, DMA_BUF_IOCTL_SYNC, &sync_end) < 0) {
		perror("Failed DMA_BUF_SYNC_END");
		return -1;
	}

	printf("DMA-BUF sync START/END succeeded\n");
	return 0;
}

static void maybe_print_virtio_features(int drm_fd, const char *driver_name)
{
	struct drm_virtgpu_getparam getparam = { 0 };

	if (strcmp(driver_name, "virtio_gpu") != 0) {
		printf("Skipping virtio specific ioctls because driver is `%s`.\n",
		       driver_name);
		return;
	}

	printf("\n--- virtio-gpu specific info ---\n");

	getparam.param = VIRTGPU_PARAM_3D_FEATURES;
	if (drmIoctl(drm_fd, DRM_IOCTL_VIRTGPU_GETPARAM, &getparam) == 0) {
		printf("3D features : %s\n", getparam.value ? "YES" : "NO");
	} else {
		printf("3D features : query failed: %s\n", strerror(errno));
	}

	memset(&getparam, 0, sizeof(getparam));
	getparam.param = VIRTGPU_PARAM_NUM_SCANOUTS;
	if (drmIoctl(drm_fd, DRM_IOCTL_VIRTGPU_GETPARAM, &getparam) == 0) {
		printf("scanouts    : %" PRIu64 "\n",
		       (uint64_t)getparam.value);
	} else {
		printf("scanouts    : query failed: %s\n", strerror(errno));
	}
}

static void write_pattern(struct test_buffer *buf)
{
	memset(buf->map_addr, 0x55, 1024);
	printf("Wrote 0x55 into the first 1024 bytes\n");

	if (((unsigned char *)buf->map_addr)[0] == 0x55 &&
	    ((unsigned char *)buf->map_addr)[1023] == 0x55) {
		printf("CPU readback validation passed\n");
	} else {
		printf("CPU readback validation failed\n");
	}
}

static void destroy_test_buffer(int drm_fd, struct test_buffer *buf)
{
	struct drm_mode_destroy_dumb destroy_arg = { 0 };

	if (buf->map_addr)
		munmap(buf->map_addr, buf->size);

	if (buf->dma_buf_fd >= 0)
		close(buf->dma_buf_fd);

	if (buf->handle) {
		destroy_arg.handle = buf->handle;
		if (drmIoctl(drm_fd, DRM_IOCTL_MODE_DESTROY_DUMB, &destroy_arg) < 0)
			perror("Failed to destroy dumb buffer");
	}
}

int main(int argc, char **argv)
{
	struct options opts = {
		.width = 512,
		.height = 512,
		.bpp = 32,
	};
	struct drm_node_info infos[32];
	const struct drm_node_info *selected = NULL;
	struct drm_node_info explicit_info = { 0 };
	struct test_buffer buf = {
		.dma_buf_fd = -1,
	};
	drmVersion *version = NULL;
	int drm_fd = -1;
	int node_count;
	int ret = EXIT_FAILURE;

	if (parse_options(argc, argv, &opts) < 0)
		return EXIT_FAILURE;

	node_count = collect_drm_nodes(infos, ARRAY_SIZE(infos));
	printf("DMA-BUF / DRM test program\n");
	printf("==========================\n");

	if (node_count == 0) {
		fprintf(stderr, "No DRM card nodes found under /dev/dri/card*\n");
		return EXIT_FAILURE;
	}

	printf("\n--- DRM nodes ---\n");
	for (int i = 0; i < node_count; ++i)
		print_node_info(&infos[i]);

	if (opts.list_only)
		return EXIT_SUCCESS;

	if (opts.device_path) {
		if (fill_node_info(opts.device_path, &explicit_info) < 0) {
			perror("Failed to inspect explicit DRM node");
			return EXIT_FAILURE;
		}
		selected = &explicit_info;
	} else {
		selected = pick_default_node(infos, node_count);
		if (!selected) {
			fprintf(stderr, "No dumb-buffer capable DRM node found\n");
			return EXIT_FAILURE;
		}
	}

	printf("\nSelected node: %s\n", selected->path);
	drm_fd = open_drm_node(selected->path);
	if (drm_fd < 0) {
		perror("Failed to open DRM node");
		return EXIT_FAILURE;
	}

	version = drmGetVersion(drm_fd);
	if (version) {
		printf("Opened DRM driver `%.*s`\n",
		       version->name_len, version->name);
		drmFreeVersion(version);
	}

	if (!selected->dumb_buffer_cap) {
		fprintf(stderr, "Selected node does not support dumb buffers\n");
		goto out;
	}

	maybe_print_virtio_features(drm_fd, selected->driver);

	if (create_test_buffer(drm_fd, &buf, &opts) < 0)
		goto out;

	if (export_buffer_as_dma_buf(drm_fd, &buf) < 0)
		goto out;

	if (map_buffer_for_cpu(drm_fd, &buf) < 0)
		goto out;

	if (test_dma_buf_sync(buf.dma_buf_fd) < 0)
		goto out;

	write_pattern(&buf);
	dump_fdinfo(buf.dma_buf_fd);

	if (!opts.skip_bufinfo)
		dump_matching_bufinfo(buf.dma_buf_inode);

	if (opts.hold_seconds > 0) {
		printf("\nHolding the allocation for %u seconds.\n",
		       opts.hold_seconds);
		printf("During this window you can inspect:\n");
		printf("  nvidia-smi --query-gpu=memory.used --format=csv,noheader\n");
		printf("  echo a | sudo -S cat /sys/kernel/debug/dma_buf/bufinfo\n");
		sleep(opts.hold_seconds);
	}

	ret = EXIT_SUCCESS;

out:
	destroy_test_buffer(drm_fd, &buf);
	close(drm_fd);
	return ret;
}
