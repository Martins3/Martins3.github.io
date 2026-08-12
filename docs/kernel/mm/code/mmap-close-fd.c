/*
 * 问题: mmap 之后, close 掉 fd, 会发生什么?
 *
 * 结论:
 * 1. 映射不会失效, 依然可以正常读写, MAP_SHARED 下 close 之后的写入依然会写回文件
 * 2. 原因: mmap 时内核执行 vma->vm_file = get_file(file), VMA 自己持有
 *    struct file 的一份引用, 与 fd 无关
 * 3. close(fd) 只是释放了 fd 对 file 的引用, file 对象因为有 VMA 的引用而存活
 * 4. close 之后 fd 号可以被其他 open 复用
 * 5. 只有 munmap 之后, remove_vma() 中 fput(vma->vm_file) 才释放最后一个引用
 * 6. 即使文件被 unlink, 只要还有 VMA 引用, inode 就不会释放, 映射依然可用
 *    (open + unlink + mmap 的经典技巧, /proc/self/maps 中显示 deleted)
 *
 * 内核代码位置:
 * - mm/mmap.c 的 mmap_region(): vma->vm_file = get_file(file)
 * - mm/mmap.c 的 remove_vma(): if (vma->vm_file) fput(vma->vm_file)
 * - 系统调用入口 __do_sys_mmap 中 fget(fd) 获得引用, mmap_region 中 get_file
 *   转给 VMA, 之后 fd 的引用就被释放, file 的生死由 VMA 决定
 *
 * 实验方法:
 * 1. mmap 一个普通文件, close(fd)
 * 2. 检查 /proc/self/maps, 映射还在
 * 3. 重新读写映射区域, 数据正常
 * 4. 重新打开文件, 验证 close 之后写入的数据依然可见
 * 5. 观察 close 之后 fd 号被复用
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../lib.h"

#define FILE_PATH "/tmp/mmap-close-fd"
#define REUSE_PATH "/tmp/mmap-close-fd-reuse"

static void show_maps(const char *tag)
{
	char cmd[256];
	snprintf(cmd, sizeof(cmd),
		 "echo '=== %s ==='; grep '/tmp/mmap-close-fd' /proc/%d/maps",
		 tag, getpid());
	if (system(cmd) == -1)
		error("system");
}

/*
 * 实验 1: MAP_SHARED + close(fd)
 * 1. open + ftruncate + mmap + 写入
 * 2. close(fd)
 * 3. 验证: 映射依然有效, 数据可读回, close 之后的新写入通过新 fd 也能看到
 */
static void test_shared_close(void)
{
	const long page_size = get_page_size();
	const size_t size = page_size * 4;

	int fd = open(FILE_PATH, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
		error("open");
	if (ftruncate(fd, size) == -1)
		error("ftruncate");

	char *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED)
		error("mmap");
	printf("fd = %d, mmap @ %p\n", fd, addr);

	strcpy(addr, "hello before close");
	show_maps("before close(fd)");

	close(fd);
	printf("closed fd %d, mapping is still alive\n", fd);
	show_maps("after close(fd)");

	/* fd 号被释放, 可以被复用 */
	int fd2 = open(REUSE_PATH, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd2 == -1)
		error("open reuse");
	printf("new open got fd = %d (reused? %s)\n", fd2,
	       fd2 == fd ? "yes" : "no");
	close(fd2);
	unlink(REUSE_PATH);

	/* 映射依然可以读写 */
	printf("read back after close: %s\n", addr);
	strcpy(addr, "written after close");
	msync(addr, size, MS_SYNC);

	/* 用新的 fd 打开同一文件, 验证写入确实到达了文件 */
	int fd3 = open(FILE_PATH, O_RDONLY);
	if (fd3 == -1)
		error("open again");
	char buf[64] = {0};
	if (read(fd3, buf, sizeof(buf)) == -1)
		error("read");
	printf("file content via new fd: %s\n", buf);
	close(fd3);

	printf("press enter to munmap\n");
	getchar();
	munmap(addr, size);
	show_maps("after munmap (mapping gone, nothing shown)");
	unlink(FILE_PATH);
}

/*
 * 实验 2: MAP_PRIVATE + close(fd)
 * close 之后写入映射, 因为是写时复制, 不会写回文件
 */
static void test_private_close(void)
{
	const long page_size = get_page_size();
	const size_t size = page_size * 4;

	int fd = open(FILE_PATH, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
		error("open");
	if (ftruncate(fd, size) == -1)
		error("ftruncate");
	if (write(fd, "file original data", 18) == -1)
		error("write");

	char *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
	if (addr == MAP_FAILED)
		error("mmap");
	close(fd);

	strcpy(addr, "private copy write");
	printf("private mapping modified: %s\n", addr);

	int fd2 = open(FILE_PATH, O_RDONLY);
	if (fd2 == -1)
		error("open again");
	char buf[64] = {0};
	if (read(fd2, buf, sizeof(buf)) == -1)
		error("read");
	printf("file content unchanged: %s\n", buf);
	close(fd2);
	munmap(addr, size);
	unlink(FILE_PATH);
}

/*
 * 实验 3: mmap + unlink + close(fd)
 * 文件被删除之后, 映射依然可用, inode 因为 VMA 的引用而存活
 * /proc/self/maps 中显示 (deleted)
 */
static void test_unlink(void)
{
	const long page_size = get_page_size();
	const size_t size = page_size * 4;

	int fd = open(FILE_PATH, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd == -1)
		error("open");
	if (ftruncate(fd, size) == -1)
		error("ftruncate");

	char *addr = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED)
		error("mmap");
	strcpy(addr, "data before unlink");

	unlink(FILE_PATH);
	close(fd);
	printf("unlinked and closed, mapping still alive\n");
	show_maps("after unlink + close (deleted)");

	printf("read back: %s\n", addr);
	strcpy(addr, "data after unlink");
	printf("write after unlink works\n");

	printf("press enter to munmap\n");
	getchar();
	munmap(addr, size);
	show_maps("after munmap (mapping gone, nothing shown)");
}

int main(int argc, char *argv[])
{
	if (argc == 1) {
		test_shared_close();
		return EXIT_SUCCESS;
	}
	int opt = atoi(argv[1]);
	switch (opt) {
	case 1:
		test_shared_close();
		break;
	case 2:
		test_private_close();
		break;
	case 3:
		test_unlink();
		break;
	default:
		printf("usage: %s [1|2|3]\n", argv[0]);
		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}
