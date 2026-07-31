/*
 * 模拟 QEMU ram_block_discard_shared_range (system/physmem.c:4096) 对
 * memfd 的 drop 操作:
 *   fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, offset, len)
 *
 * 测试内容:
 * 1. memfd 分配 10G, 以 gran 为粒度随机分配一半的块 (alloc 粒度 = gran)
 * 2. 逐块 (gran) punch hole, 统计总耗时 / 单块延迟 / 带宽
 *    gran 分别取 4K 和 64K
 * 3. 一次性 punch hole 整个 10G 范围, 统计带宽
 * 4. 用 fstat st_blocks 验证内存确实被释放
 */
#include "lib.h"
#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <linux/falloc.h>

static uint64_t prng_state = 0x123456789abcdef0;

static inline uint64_t xorshift64(void)
{
	uint64_t x = prng_state;
	x ^= x << 13;
	x ^= x >> 7;
	x ^= x << 17;
	prng_state = x;
	return x;
}

static inline double now_sec(void)
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static inline long memfd_blocks(int fd)
{
	struct stat st;
	if (fstat(fd, &st))
		error("fstat");
	return st.st_blocks * 512;
}

// 洗牌块号: 前一半是"被分配"的块, 顺序也是随机的
static void shuffle(uint64_t *order, long n)
{
	for (long i = 0; i < n; i++)
		order[i] = i;
	for (long i = n - 1; i > 0; i--) {
		uint64_t j = xorshift64() % (i + 1);
		uint64_t tmp = order[i];
		order[i] = order[j];
		order[j] = tmp;
	}
}

// 按 gran 粒度分配前 nr_blocks 个 (已洗牌的) 块: 块内每个 4K 页都 touch
static void populate(char *base, uint64_t *order, long nr_blocks, long gran)
{
	long page_size = get_page_size();
	for (long i = 0; i < nr_blocks; i++)
		for (long off = 0; off < gran; off += page_size)
			base[order[i] * gran + off] = 1;
}

static void test_punch_gran(char *base, int fd, long size, long gran,
			    uint64_t *order)
{
	long nr_blocks = size / gran;
	long nr_populated = nr_blocks / 2;

	shuffle(order, nr_blocks);

	double t0 = now_sec();
	populate(base, order, nr_populated, gran);
	printf("populate %ld random %ldK blocks: %.2f s, allocated %ld MB\n",
	       nr_populated, gran >> 10, now_sec() - t0, memfd_blocks(fd) >> 20);

	// 逐块 punch hole, 模拟 QEMU 按块 discard
	t0 = now_sec();
	for (long i = 0; i < nr_populated; i++) {
		if (fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE,
			      order[i] * gran, gran))
			error("fallocate punch hole");
	}
	double elapsed = now_sec() - t0;
	double mb = (double)nr_populated * gran / (1 << 20);
	printf("punch hole per %ldK block: %ld calls in %.2f s\n"
	       "  -> %.2f us/call, %.2f GB/s, allocated %ld MB\n",
	       gran >> 10, nr_populated, elapsed,
	       elapsed * 1e6 / nr_populated, mb / 1024 / elapsed,
	       memfd_blocks(fd) >> 20);
}

int main(int argc, char *argv[])
{
	long size = get_size(10, 'G');
	long page_size = get_page_size();
	int fd;

	char *base = mmap_region_memfd(size, &fd);

	uint64_t *order = malloc(size / page_size * sizeof(uint64_t));
	if (!order)
		error("malloc");

	// 测试 1: alloc/free 粒度都是 4K
	test_punch_gran(base, fd, size, page_size, order);

	// 测试 2: alloc/free 粒度都是 64K
	test_punch_gran(base, fd, size, get_size(64, 'K'), order);

	// 测试 3: 一次性 punch hole 整个范围 (内部一半 4K 页是 hole)
	shuffle(order, size / page_size);
	populate(base, order, size / page_size / 2, page_size);

	double t0 = now_sec();
	if (fallocate(fd, FALLOC_FL_PUNCH_HOLE | FALLOC_FL_KEEP_SIZE, 0, size))
		error("fallocate punch hole whole");
	double elapsed = now_sec() - t0;
	printf("punch hole whole %ld GB at once: %.2f s -> %.2f GB/s, allocated %ld MB\n",
	       size >> 30, elapsed, (double)size / (1 << 30) / elapsed,
	       memfd_blocks(fd) >> 20);

	return 0;
}
