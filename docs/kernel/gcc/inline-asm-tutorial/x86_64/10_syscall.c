/*
 * 示例 10: 系统调用 (System Call)
 *
 * 知识点:
 * - x86_64 syscall 调用约定
 * - syscall 指令的使用
 * - 错误处理: 负数返回值表示错误
 * - 实际应用: 实现快速系统调用
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

/* x86_64 Linux syscall 号 */
#define SYS_WRITE  1
#define SYS_EXIT   60
#define SYS_GETPID 39
#define SYS_GETUID 102
#define SYS_READ   0
#define SYS_OPEN   2
#define SYS_CLOSE  3

/* 内联汇编实现系统调用
 * x86_64 syscall 约定:
 *   rax = syscall number
 *   rdi = arg1
 *   rsi = arg2
 *   rdx = arg3
 *   r10 = arg4
 *   r8  = arg5
 *   r9  = arg6
 *   rcx, r11 被内核破坏
 *   rax = return value (负数表示错误)
 */

/* 0 参数系统调用 */
static inline long sys_call0(long n)
{
	unsigned long ret;

	__asm__ __volatile__(
		"syscall"
		: "=a" (ret)
		: "a" (n)
		: "rcx", "r11", "memory"
		);

	return (long)ret;
}

/* 1 参数系统调用 */
static inline long sys_call1(long n, long a1)
{
	unsigned long ret;

	__asm__ __volatile__(
		"syscall"
		: "=a" (ret)
		: "a" (n), "D" (a1)
		: "rcx", "r11", "memory"
		);

	return (long)ret;
}

/* 2 参数系统调用 */
static inline long sys_call2(long n, long a1, long a2)
{
	unsigned long ret;

	__asm__ __volatile__(
		"syscall"
		: "=a" (ret)
		: "a" (n), "D" (a1), "S" (a2)
		: "rcx", "r11", "memory"
		);

	return (long)ret;
}

/* 3 参数系统调用 */
static inline long sys_call3(long n, long a1, long a2, long a3)
{
	unsigned long ret;

	__asm__ __volatile__(
		"syscall"
		: "=a" (ret)
		: "a" (n), "D" (a1), "S" (a2), "d" (a3)
		: "rcx", "r11", "memory"
		);

	return (long)ret;
}

/* 包装函数 */
static inline long sys_write(unsigned int fd, const char *buf, size_t count)
{
	return sys_call3(SYS_WRITE, fd, (long)buf, count);
}

static inline long sys_getpid(void)
{
	return sys_call0(SYS_GETPID);
}

static inline long sys_getuid(void)
{
	return sys_call0(SYS_GETUID);
}

static inline long sys_read(unsigned int fd, char *buf, size_t count)
{
	return sys_call3(SYS_READ, fd, (long)buf, count);
}

/* 自定义 write 实现 */
static size_t my_write(int fd, const void *buf, size_t count)
{
	long ret = sys_write(fd, buf, count);
	if (ret < 0) {
		errno = -ret;
		return -1;
	}
	return (size_t)ret;
}

int main(void)
{
	long ret;

	/* ===== 测试 1: write 系统调用 ===== */
	const char msg1[] = "Testing sys_write via inline asm\n";
	ret = sys_write(1, msg1, sizeof(msg1) - 1);
	printf("write returned: %ld (expected: %zu)\n", ret, sizeof(msg1) - 1);
	assert(ret == (long)(sizeof(msg1) - 1));

	/* ===== 测试 2: getpid 系统调用 ===== */
	ret = sys_getpid();
	printf("getpid returned: %ld\n", ret);
	assert(ret > 0);  /* PID 应该是正数 */

	/* 验证与库函数一致 */
	pid_t libc_pid = getpid();
	printf("libc getpid: %d\n", libc_pid);
	assert(ret == libc_pid);

	/* ===== 测试 3: getuid 系统调用 ===== */
	ret = sys_getuid();
	printf("getuid returned: %ld\n", ret);
	/* UID 可能是 0 (root) 或其他值 */

	uid_t libc_uid = getuid();
	printf("libc getuid: %d\n", libc_uid);
	assert(ret == libc_uid);

	/* ===== 测试 4: 使用 my_write ===== */
	const char msg2[] = "Using my_write wrapper\n";
	size_t written = my_write(1, msg2, sizeof(msg2) - 1);
	printf("my_write returned: %zu\n", written);
	assert(written == sizeof(msg2) - 1);

	/* ===== 测试 5: 完整 syscall 实现演示 ===== */
	/* 实现一个完整的 write 循环 */
	const char long_msg[] = "This is a longer message that demonstrates "
	                        "repeated system calls via inline assembly\n";
	size_t total_len = sizeof(long_msg) - 1;
	size_t written_total = 0;

	while (written_total < total_len) {
		ret = sys_write(1, long_msg + written_total, total_len - written_total);
		if (ret < 0) {
			printf("write error: %ld\n", ret);
			break;
		}
		written_total += ret;
	}

	printf("Total bytes written: %zu\n", written_total);
	assert(written_total == total_len);

	/* ===== 测试 6: 错误处理 ===== */
	/* 尝试写入无效 fd */
	ret = sys_write(999, "test", 4);
	printf("write to invalid fd returned: %ld (expected: negative)\n", ret);
	assert(ret < 0);  /* 应该是错误 */

	/* ===== 测试 7: 直接内联汇编 syscall ===== */
	/* 不使用包装函数，直接调用 */
	const char direct[] = "Direct syscall without wrapper\n";

	__asm__ __volatile__(
		"movq $1, %%rax\n\t"    /* SYS_WRITE */
		"movq $1, %%rdi\n\t"    /* stdout */
		"movq %0, %%rsi\n\t"    /* buf */
		"movq %1, %%rdx\n\t"    /* count */
		"syscall"
		:
		: "r" (direct), "r" ((long)(sizeof(direct) - 1))
		: "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
		);

	printf("\n=== All syscall tests passed! ===\n");
	return 0;
}
