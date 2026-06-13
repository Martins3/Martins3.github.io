#include "common.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void test_parse_size(void)
{
	uint64_t value = 0;

	assert(fb_parse_size("4096", &value) == 0);
	assert(value == 4096);
	assert(fb_parse_size("4k", &value) == 0);
	assert(value == 4096);
	assert(fb_parse_size("8M", &value) == 0);
	assert(value == 8ULL * 1024 * 1024);
	assert(fb_parse_size("2g", &value) == 0);
	assert(value == 2ULL * 1024 * 1024 * 1024);

	assert(fb_parse_size("", &value) == -1);
	assert(fb_parse_size("12x", &value) == -1);
	assert(fb_parse_size("1kb", &value) == -1);
	assert(fb_parse_size("18446744073709551615g", &value) == -1);
}

static void test_format_caps(void)
{
	char buf[256];
	uint64_t caps = FB_CAP_ASYNC_READ | FB_CAP_BIG_WRITES |
			FB_CAP_PASSTHROUGH | FB_CAP_OVER_IO_URING;

	fb_format_caps(caps, buf, sizeof(buf));
	assert(strstr(buf, "ASYNC_READ") != NULL);
	assert(strstr(buf, "BIG_WRITES") != NULL);
	assert(strstr(buf, "PASSTHROUGH") != NULL);
	assert(strstr(buf, "OVER_IO_URING") != NULL);
	assert(strstr(buf, "WRITEBACK_CACHE") == NULL);

	fb_format_caps(0, buf, sizeof(buf));
	assert(strcmp(buf, "none") == 0);
}

static void test_rate_mib(void)
{
	double mib = fb_rate_mib_per_sec(64ULL * 1024 * 1024, 2000000000ULL);
	assert(mib > 31.99 && mib < 32.01);
	assert(fb_rate_mib_per_sec(1024, 0) == 0.0);
}

static void test_build_mount_opts(void)
{
	char buf[128];

	assert(fb_build_fuse_mount_opts("bench.img", buf, sizeof(buf)) == 0);
	assert(strcmp(buf, "fsname=bench.img,subtype=fuse-bench") == 0);
	assert(strstr(buf, "fd=") == NULL);
	assert(strstr(buf, "user_id=") == NULL);

	assert(fb_build_fuse_mount_opts("a,b", buf, sizeof(buf)) == -1);
	assert(fb_build_fuse_mount_opts("bench.img", buf, 8) == -1);
}

static void test_possible_cpu_count(void)
{
	unsigned int count = fb_possible_cpu_count();

	assert(count > 0);
	assert(count < 4096);
}

int main(void)
{
	test_parse_size();
	test_format_caps();
	test_rate_mib();
	test_build_mount_opts();
	test_possible_cpu_count();
	puts("test_common: ok");
	return 0;
}
