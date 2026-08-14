#define _GNU_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "lib.h"

/*
 * 模拟 QEMU memory-backend-memfd 的映射行为，复现/验证 guest RAM 拿不到 THP
 * 的根因：shmem(memfd) 的 THP 要求 VMA 起始地址对 2MB 对齐，而 QEMU 的
 * memfd 后端只按 4KB 对齐映射。
 *
 * 对照 QEMU 源码:
 * - util/mmap-alloc.c:qemu_ram_mmap()      -> 下面的 qemu_ram_mmap()
 * - system/physmem.c:ram_block_add()       -> madvise(QEMU_MADV_HUGEPAGE)
 * - system/physmem.c:file_ram_alloc()      -> mr->align = MAX(page_size, mr->align)
 *   memory-backend-memfd 没设 align -> 4KB；匿名后端 -> QEMU_VMALLOC_ALIGN = 2MB
 *
 * 对照内核源码:
 * - include/linux/huge_mm.h:thp_vma_suitable_order()  对齐检查
 * - mm/shmem.c:shmem_allowable_huge_orders()          只有 2MB 档继承全局配置
 *
 * 结论: 4KB 对齐 -> THPeligible=0 / KernelPageSize=4kB；2MB 对齐 -> THP 生效。
 */

#define PMD_SIZE (2UL * 1024 * 1024)
#define ALIGN_UP(x, a) (((x) + (a) - 1) & ~((a) - 1))

/* 直译 QEMU util/mmap-alloc.c:qemu_ram_mmap() */
static void *qemu_ram_mmap(int fd, size_t size, size_t align)
{
	size_t total = size + align;
	void *guardptr = mmap(NULL, total, PROT_NONE,
			      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (guardptr == MAP_FAILED)
		error("mmap reserve");

	size_t offset = ALIGN_UP((uintptr_t)guardptr, align) - (uintptr_t)guardptr;
	void *ptr = mmap((char *)guardptr + offset, size,
			 PROT_READ | PROT_WRITE, MAP_FIXED | MAP_SHARED, fd, 0);
	if (ptr == MAP_FAILED)
		error("mmap activate");

	return ptr;
}

/* QEMU backends/hostmem-memfd.c:memfd_backend_memory_alloc() */
static int qemu_memfd_create(size_t size)
{
	int fd = memfd_create("qemu-sim", MFD_ALLOW_SEALING);
	if (fd == -1)
		error("memfd_create");

	if (ftruncate(fd, size) == -1)
		error("ftruncate");

	/* QEMU 默认 seal = true */
	if (fcntl(fd, F_ADD_SEALS, F_SEAL_GROW | F_SEAL_SHRINK | F_SEAL_SEAL) == -1)
		error("F_ADD_SEALS");

	return fd;
}

/* 打印 /proc/self/smaps 中包含 ptr 的那一个 VMA 段 */
static void dump_smaps(void *ptr)
{
	FILE *f = fopen("/proc/self/smaps", "r");
	if (!f)
		error("fopen smaps");

	char line[1024];
	uintptr_t target = (uintptr_t)ptr;
	bool show = false;

	while (fgets(line, sizeof(line), f)) {
		unsigned long start, end;
		if (sscanf(line, "%lx-%lx", &start, &end) == 2)
			show = (target >= start && target < end);
		if (show)
			fputs(line, stdout);
		if (show && line[0] == '\n')
			break;
	}
	fclose(f);
}

static void show_sysfs(const char *path, const char *name)
{
	FILE *f = fopen(path, "r");
	char buf[256];
	printf("%-16s: ", name);
	if (f && fgets(buf, sizeof(buf), f))
		printf("%s", buf);
	else
		printf("?\n");
	if (f)
		fclose(f);
}

static void run(size_t size, size_t align, const char *label)
{
	int fd = qemu_memfd_create(size);
	void *ptr = qemu_ram_mmap(fd, size, align);

	/* QEMU system/physmem.c:ram_block_add() */
	if (madvise(ptr, size, MADV_HUGEPAGE) == -1)
		error("madvise");

	/* guest 启动后按需 fault 全部页 */
	touch((char *)ptr, get_page_size(), size, true);

	printf("==== %s ====\n", label);
	printf("ptr=%p 2MB-aligned=%s\n", ptr,
	       (((uintptr_t)ptr & (PMD_SIZE - 1)) == 0) ? "yes" : "no");
	dump_smaps(ptr);
	printf("\n");

	munmap(ptr, size);
	close(fd);
}

int main(int argc, char *argv[])
{
	long size = get_size(512, 'M');
	if (argc > 1)
		size = get_size(atoi(argv[1]), 'M');

	show_sysfs("/sys/kernel/mm/transparent_hugepage/enabled", "enabled");
	show_sysfs("/sys/kernel/mm/transparent_hugepage/shmem_enabled",
		   "shmem_enabled");
	show_sysfs("/sys/kernel/mm/transparent_hugepage/hugepages-2048kB/shmem_enabled",
		   "2048kB/shmem");
	printf("\n");

	/* memory-backend-memfd: mr->align 缺省 -> 4KB，复现问题 */
	run(size, get_page_size(), "qemu memory-backend-memfd (align=4K)");

	/* 修复后 / 匿名后端: QEMU_VMALLOC_ALIGN = 2MB */
	run(size, PMD_SIZE, "fixed (align=2M)");

	return 0;
}
