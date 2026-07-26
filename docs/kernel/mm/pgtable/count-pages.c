#define _GNU_SOURCE
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>

/*
 * 如何获取 cgroup inode：
 *   stat -c '%i' /sys/fs/cgroup/.../xxx.scope
 * 或
 *   ls -id /sys/fs/cgroup/.../xxx.scope | awk '{print $1}'
 *
 * 将得到的 inode 号作为本程序的参数传入，用于 /proc/kpagecgroup 匹配。
 *
 *  stat -c "%i" /sys/fs/cgroup/user.slice/user-1000.slice/user@1000.service/app.slice/tmux-spawn-8f98d690-2ebf-4181-a735-fe10d8fbf7d4.scope
 *  sudo docs/kernel/mm/pgtable/count.out 572522
 *
 * 大致效果为:
 * scanned_pages=34078720 matched_pages=527215 matched_mib=2059.4 elapsed=1.56
 * pgtable_only_pages=525340 pgtable_only_mib=2052.1
 * anon_pages=1122 anon_mib=4.4
 * swapbacked_nonanon_pages=0 swapbacked_nonanon_mib=0.0
 * slab_pages=0 slab_mib=0.0
 * none_pages=4 none_mib=0.0
 * bit REFERENCED     pages=         521 mib=       2.0
 * bit UPTODATE       pages=        1871 mib=       7.3
 * bit LRU            pages=        1871 mib=       7.3
 * bit RECLAIM        pages=           1 mib=       0.0
 * bit MMAP           pages=        1126 mib=       4.4
 * bit ANON           pages=        1122 mib=       4.4
 * bit SWAPBACKED     pages=        1122 mib=       4.4
 * bit KSM            pages=        1122 mib=       4.4
 * bit PGTABLE        pages=      525340 mib=    2052.1
 *
 * 相当于是遍历所有的 page ，然后看看是不是当前 cgroup 的
 * 的页面，然后看看是什么 flags 的页面
 */
#define CHUNK (1 << 20)
struct bit {
	int bit;
	const char *name;
} bits[] = { { 0, "LOCKED" },	      { 1, "ERROR" },
	     { 2, "REFERENCED" },     { 3, "UPTODATE" },
	     { 4, "DIRTY" },	      { 5, "LRU" },
	     { 6, "ACTIVE" },	      { 7, "SLAB" },
	     { 8, "WRITEBACK" },      { 9, "RECLAIM" },
	     { 10, "BUDDY" },	      { 11, "MMAP" },
	     { 12, "ANON" },	      { 13, "SWAPCACHE" },
	     { 14, "SWAPBACKED" },    { 15, "COMPOUND_HEAD" },
	     { 16, "COMPOUND_TAIL" }, { 17, "HUGE" },
	     { 18, "UNEVICTABLE" },   { 19, "HWPOISON" },
	     { 20, "NOPAGE" },	      { 21, "KSM" },
	     { 22, "THP" },	      { 23, "OFFLINE" },
	     { 24, "ZERO_PAGE" },     { 25, "IDLE" },
	     { 26, "PGTABLE" },	      { -1, NULL } };
int main(int argc, char **argv)
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s <cgroup_inode>\n", argv[0]);
		return 2;
	}
	uint64_t target = strtoull(argv[1], NULL, 0);
	int fcg = open("/proc/kpagecgroup", O_RDONLY),
	    ffl = open("/proc/kpageflags", O_RDONLY);
	if (fcg < 0 || ffl < 0) {
		perror("open");
		return 1;
	}
	uint64_t *cg = malloc(CHUNK * 8), *fl = malloc(CHUNK * 8);
	if (!cg || !fl) {
		perror("malloc");
		return 1;
	}
	uint64_t bitcnt[64] = { 0 }, matched = 0, total = 0, none = 0;
	uint64_t pgtable_only = 0, anon = 0, shmem = 0, slab = 0;
	struct timespec ts0, ts1;
	clock_gettime(CLOCK_MONOTONIC, &ts0);
	for (;;) {
		ssize_t n = read(fcg, cg, CHUNK * 8);
		if (n <= 0)
			break;
		ssize_t m = read(ffl, fl, n);
		if (m != n) {
			fprintf(stderr, "short flags read\n");
			return 1;
		}
		size_t nr = n / 8;
		total += nr;
		for (size_t i = 0; i < nr; i++) {
			if (cg[i] != target)
				continue;
			uint64_t f = fl[i];
			matched++;
			if (!f)
				none++;
			for (int b = 0; bits[b].bit >= 0; b++)
				if (f & (1ULL << bits[b].bit))
					bitcnt[bits[b].bit]++;
			if (f == (1ULL << 26))
				pgtable_only++;
			if (f & (1ULL << 12))
				anon++;
			if ((f & (1ULL << 14)) && !(f & (1ULL << 12)))
				shmem++;
			if (f & (1ULL << 7))
				slab++;
		}
	}
	clock_gettime(CLOCK_MONOTONIC, &ts1);
	double sec =
		(ts1.tv_sec - ts0.tv_sec) + (ts1.tv_nsec - ts0.tv_nsec) / 1e9;
	printf("scanned_pages=%" PRIu64 " matched_pages=%" PRIu64
	       " matched_mib=%.1f elapsed=%.2f\n",
	       total, matched, matched * 4096.0 / 1024 / 1024, sec);
	printf("pgtable_only_pages=%" PRIu64 " pgtable_only_mib=%.1f\n",
	       pgtable_only, pgtable_only * 4096.0 / 1024 / 1024);
	printf("anon_pages=%" PRIu64 " anon_mib=%.1f\n", anon,
	       anon * 4096.0 / 1024 / 1024);
	printf("swapbacked_nonanon_pages=%" PRIu64
	       " swapbacked_nonanon_mib=%.1f\n",
	       shmem, shmem * 4096.0 / 1024 / 1024);
	printf("slab_pages=%" PRIu64 " slab_mib=%.1f\n", slab,
	       slab * 4096.0 / 1024 / 1024);
	printf("none_pages=%" PRIu64 " none_mib=%.1f\n", none,
	       none * 4096.0 / 1024 / 1024);
	for (int b = 0; bits[b].bit >= 0; b++)
		if (bitcnt[bits[b].bit])
			printf("bit %-14s pages=%12" PRIu64 " mib=%10.1f\n",
			       bits[b].name, bitcnt[bits[b].bit],
			       bitcnt[bits[b].bit] * 4096.0 / 1024 / 1024);
	return 0;
}
