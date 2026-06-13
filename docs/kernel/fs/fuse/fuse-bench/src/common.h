#ifndef FUSE_BENCH_COMMON_H
#define FUSE_BENCH_COMMON_H

#include <stddef.h>
#include <stdint.h>

#define FB_CAP_ASYNC_READ (1ULL << 0)
#define FB_CAP_BIG_WRITES (1ULL << 5)
#define FB_CAP_WRITEBACK_CACHE (1ULL << 16)
#define FB_CAP_ASYNC_DIO (1ULL << 15)
#define FB_CAP_DIRECT_IO_ALLOW_MMAP (1ULL << 36)
#define FB_CAP_PASSTHROUGH (1ULL << 37)
#define FB_CAP_OVER_IO_URING (1ULL << 41)

int fb_parse_size(const char *text, uint64_t *value);
void fb_format_caps(uint64_t caps, char *buf, size_t size);
double fb_rate_mib_per_sec(uint64_t bytes, uint64_t nsec);
uint64_t fb_now_nsec(void);
int fb_build_fuse_mount_opts(const char *fsname, char *buf, size_t size);
unsigned int fb_possible_cpu_count(void);

#endif
