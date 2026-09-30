// mmap 系统调用的 flags 基本理解
// <!-- adeaecbd-fbb5-471d-84c8-f88d45f6f1a5 -->
//
// 如果没有 MAP_ANONYMOUS ，那么就会检查 fd ，所以这两个都会失败
// void *ptr2 = mmap_region(MAP_SIZE, -1, MAP_SHARED);
// void *ptr = mmap_region(MAP_SIZE, -1, MAP_PRIVATE);
//
// MAP_SHARED 和 MAP_PRIVATE 必需带一个
//
// 所以，这个 flags 就很简单了:
//
// MAP_PRIVATE/MAP_SHARED 是一组，
// MAP_ANONYMOUS 是一组，他们是正交的关系。
//
// 下面是实验跑出来的结论(不是拍脑袋写的):
//
// 实验 1 : 四种组合 mmap 都成功，两组 flags 确实正交
//          (fd=-1 且不带 MAP_ANONYMOUS -> EBADF，两个都不带 -> EINVAL)
// 实验 2 : 同进程再 mmap 一次，是不是同一块物理内存
//          只有 (MAP_SHARED + fd) 是；MAP_ANONYMOUS 每次 mmap 都是全新的一块，
//          没有名字/句柄能指回同一块 —— 这就是 "不知道用什么方法来共享"
// 实验 3 : fork 之后写是否互相可见
//          MAP_SHARED 可见，MAP_PRIVATE 是写时复制
// 实验 4 : 子进程不继承映射，自己 open 同一个文件再 mmap
//          MAP_SHARED + fd 依然可见 —— 共享靠的是文件，不是 fork
//          MAP_ANONYMOUS 根本没有文件可以 open ，无关进程无从共享
//
// 所以 "共享" 有两条路:
//   - MAP_SHARED + fd        : 任何打开同一个文件的进程都能共享(和 fork 无关)
//   - MAP_SHARED + MAP_ANON  : 只能靠 fork 分给子孙进程，无关进程没戏
//
// | 类型        | fd                 | MAP_ANONYMOUS            |
// |-------------|--------------------|--------------------------|
// | MAP_SHARED  | ok，靠文件共享      | ok，只能靠 fork 共享     |
// | MAP_PRIVATE | ok，写时复制        | ok，写时复制             |
//
// 其中 MAP_SHARED + MAP_ANONYMOUS 就是 rmap 实现最痛苦环节，anonymous 映射，但是 fork 之后依旧共享。
#define _GNU_SOURCE
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

#define PAGE_SIZE 4096
#define MAP_SIZE  PAGE_SIZE

#define MARK_A 0xaa
#define MARK_B 0xbb

struct combo {
	const char *desc;
	int use_fd; // 1: 用文件 fd, 0: 用 MAP_ANONYMOUS
	int flags; // MAP_SHARED 或 MAP_PRIVATE
};

static struct combo combos[] = {
	{ "MAP_SHARED  + fd           ", 1, MAP_SHARED },
	{ "MAP_SHARED  + MAP_ANONYMOUS ", 0, MAP_SHARED | MAP_ANONYMOUS },
	{ "MAP_PRIVATE + fd           ", 1, MAP_PRIVATE },
	{ "MAP_PRIVATE + MAP_ANONYMOUS ", 0, MAP_PRIVATE | MAP_ANONYMOUS },
};

#define NCOMBO (sizeof(combos) / sizeof(combos[0]))

// 每个 case 用全新的文件，避免上一个 case 通过 page cache 污染结果
static int open_file(void)
{
	char path[] = "/tmp/mmap-flags-XXXXXX";
	int fd = mkstemp(path);
	if (fd == -1) {
		perror("mkstemp");
		exit(EXIT_FAILURE);
	}
	unlink(path);
	if (ftruncate(fd, MAP_SIZE) == -1) {
		perror("ftruncate");
		exit(EXIT_FAILURE);
	}
	return fd;
}

static void *do_map(struct combo *c, int fd)
{
	return mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, c->flags,
		    c->use_fd ? fd : -1, 0);
}

// 通过 /proc/self/pagemap 查虚拟地址对应的物理页帧号(PFN)
// 没权限时内核会把 PFN 抹成 0，返回 0 表示查不到
static unsigned long virt_to_pfn(void *addr)
{
	int fd = open("/proc/self/pagemap", O_RDONLY);
	if (fd == -1)
		return 0;

	unsigned long entry = 0;
	unsigned long vpn = (unsigned long)addr / PAGE_SIZE;
	if (pread(fd, &entry, sizeof(entry), vpn * sizeof(entry)) !=
	    sizeof(entry))
		entry = 0;
	close(fd);

	// bit 63: page present, bit 0-54: PFN
	if (!(entry & (1UL << 63)))
		return 0;
	return entry & ((1UL << 55) - 1);
}

// 实验 1 : 各种 flags 组合 mmap 到底成不成功
static void exp_mmap_ok(void)
{
	printf("=== 实验 1 : mmap 能不能成功 ===\n");
	for (unsigned int i = 0; i < NCOMBO; i++) {
		int fd = open_file();
		void *ptr = do_map(&combos[i], fd);
		printf("%s : %s\n", combos[i].desc,
		       ptr == MAP_FAILED ? strerror(errno) : "ok");
		if (ptr != MAP_FAILED)
			munmap(ptr, MAP_SIZE);
		close(fd);
	}

	// 没有 MAP_ANONYMOUS 时 kernel 会去检查 fd
	void *ptr = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, -1,
			 0);
	printf("MAP_SHARED   + fd=-1 (无 MAP_ANONYMOUS) : %s\n",
	       ptr == MAP_FAILED ? strerror(errno) : "ok(意外!)");

	ptr = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE, -1, 0);
	printf("MAP_PRIVATE  + fd=-1 (无 MAP_ANONYMOUS) : %s\n",
	       ptr == MAP_FAILED ? strerror(errno) : "ok(意外!)");

	// MAP_SHARED / MAP_PRIVATE 必需带一个
	ptr = mmap(NULL, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS, -1, 0);
	printf("MAP_ANONYMOUS + 两个都不带             : %s\n",
	       ptr == MAP_FAILED ? strerror(errno) : "ok(意外!)");
}

// 实验 3 : fork 出子进程之后，写互相可见吗?
//          父进程写 MARK_A，fork 后子进程改成 MARK_B，父进程再看这个字节
static void exp_fork_share(void)
{
	printf("\n=== 实验 2 : fork 之后写是否互相可见 ===\n");

	for (unsigned int i = 0; i < NCOMBO; i++) {
		int fd = open_file();
		char *p = do_map(&combos[i], fd);
		p[0] = MARK_A;

		pid_t pid = fork();
		if (pid < 0) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		if (pid == 0) {
			p[0] = MARK_B;
			_exit(0);
		}
		int st;
		waitpid(pid, &st, 0);

		printf("%s : 父进程读到 0x%x -> %s\n", combos[i].desc,
		       (unsigned char)p[0],
		       (unsigned char)p[0] == MARK_B ? "共享(子进程的写可见)" :
						       "写时复制(子进程的写不可见)");

		munmap(p, MAP_SIZE);
		close(fd);
	}
}

int main(void)
{
	exp_mmap_ok();
	exp_fork_share();
	return EXIT_SUCCESS;
}
