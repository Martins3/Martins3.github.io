#include "common.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

struct cap_name {
	uint64_t bit;
	const char *name;
};

static const struct cap_name cap_names[] = {
	{ FB_CAP_ASYNC_READ, "ASYNC_READ" },
	{ FB_CAP_BIG_WRITES, "BIG_WRITES" },
	{ FB_CAP_ASYNC_DIO, "ASYNC_DIO" },
	{ FB_CAP_WRITEBACK_CACHE, "WRITEBACK_CACHE" },
	{ FB_CAP_DIRECT_IO_ALLOW_MMAP, "DIRECT_IO_ALLOW_MMAP" },
	{ FB_CAP_PASSTHROUGH, "PASSTHROUGH" },
	{ FB_CAP_OVER_IO_URING, "OVER_IO_URING" },
};

int fb_parse_size(const char *text, uint64_t *value)
{
	char *end = NULL;
	unsigned long long base;
	uint64_t multiplier = 1;

	if (text == NULL || text[0] == '\0' || value == NULL)
		return -1;
	if (isspace((unsigned char)text[0]) || text[0] == '-')
		return -1;

	errno = 0;
	base = strtoull(text, &end, 10);
	if (errno != 0 || end == text)
		return -1;

	if (*end != '\0') {
		if (end[1] != '\0')
			return -1;
		switch (*end) {
		case 'k':
		case 'K':
			multiplier = 1024ULL;
			break;
		case 'm':
		case 'M':
			multiplier = 1024ULL * 1024;
			break;
		case 'g':
		case 'G':
			multiplier = 1024ULL * 1024 * 1024;
			break;
		default:
			return -1;
		}
	}

	if (base > UINT64_MAX / multiplier)
		return -1;
	*value = (uint64_t)base * multiplier;
	return 0;
}

void fb_format_caps(uint64_t caps, char *buf, size_t size)
{
	size_t used = 0;
	int first = 1;

	if (buf == NULL || size == 0)
		return;
	buf[0] = '\0';

	for (size_t i = 0; i < sizeof(cap_names) / sizeof(cap_names[0]); i++) {
		int n;

		if ((caps & cap_names[i].bit) == 0)
			continue;

		n = snprintf(buf + used, size - used, "%s%s",
			     first ? "" : ",", cap_names[i].name);
		if (n < 0)
			break;
		if ((size_t)n >= size - used) {
			buf[size - 1] = '\0';
			return;
		}
		used += (size_t)n;
		first = 0;
	}

	if (first)
		snprintf(buf, size, "none");
}

double fb_rate_mib_per_sec(uint64_t bytes, uint64_t nsec)
{
	if (nsec == 0)
		return 0.0;
	return ((double)bytes / (1024.0 * 1024.0)) / ((double)nsec / 1.0e9);
}

uint64_t fb_now_nsec(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0)
		return 0;
	return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int fb_build_fuse_mount_opts(const char *fsname, char *buf, size_t size)
{
	int n;

	if (!fsname || !buf || size == 0 || fsname[0] == '\0')
		return -1;
	if (strchr(fsname, ',') || strchr(fsname, '\n') ||
	    strchr(fsname, '\t') || strchr(fsname, '\\'))
		return -1;

	n = snprintf(buf, size, "fsname=%s,subtype=fuse-bench", fsname);
	if (n < 0 || (size_t)n >= size)
		return -1;
	return 0;
}

unsigned int fb_possible_cpu_count(void)
{
	long count = sysconf(_SC_NPROCESSORS_CONF);

	if (count <= 0)
		return 1;
	if (count > 4096)
		return 4096;
	return (unsigned int)count;
}
