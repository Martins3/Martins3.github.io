/*
 * 示例 10: 系统调用 (System Call) (ARM64/AArch64)
 *
 * 知识点:
 * - ARM64 svc 指令 (Supervisor Call)
 * - 系统调用号放在 x8 寄存器
 * - 参数放在 x0-x5 寄存器
 * - 返回值在 x0
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

/* ARM64 Linux syscall 号 */
#define SYS_WRITE  64
#define SYS_EXIT   93
#define SYS_GETPID 172
#define SYS_GETUID 174
#define SYS_READ   63

/* ARM64 syscall 包装函数
 * 约定:
 *   x8 = syscall number
 *   x0-x5 = arguments
 *   svc #0 = 触发系统调用
 *   x0 = return value
 */

/* 0 参数系统调用 */
static inline long sys_call0(long n)
{
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0");

	__asm__ __volatile__(
		"svc #0"
		: "=r" (x0)
		: "r" (x8)
		: "memory", "cc"
		);

	return x0;
}

/* 1 参数系统调用 */
static inline long sys_call1(long n, long a1)
{
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0") = a1;

	__asm__ __volatile__(
		"svc #0"
		: "=r" (x0)
		: "r" (x8), "0" (x0)
		: "memory", "cc"
		);

	return x0;
}

/* 3 参数系统调用 */
static inline long sys_call3(long n, long a1, long a2, long a3)
{
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0") = a1;
	register long x1 __asm__("x1") = a2;
	register long x2 __asm__("x2") = a3;

	__asm__ __volatile__(
		"svc #0"
		: "=r" (x0)
		: "r" (x8), "r" (x0), "r" (x1), "r" (x2)
		: "memory", "cc"
		);

	return x0;
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
	const char msg1[] = "Testing sys_write via inline asm (ARM64)\n";
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

	uid_t libc_uid = getuid();
	printf("libc getuid: %d\n", libc_uid);
	assert(ret == libc_uid);

	/* ===== 测试 4: 使用 my_write ===== */
	const char msg2[] = "Using my_write wrapper\n";
	size_t written = my_write(1, msg2, sizeof(msg2) - 1);
	printf("my_write returned: %zu\n", written);
	assert(written == sizeof(msg2) - 1);

	/* ===== 测试 5: 完整 syscall 实现演示 ===== */
	const char long_msg[] = "This is a longer message that demonstrates "
	                        "repeated system calls via inline assembly (ARM64)\n";
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
	const char direct[] = "Direct syscall without wrapper (ARM64)\n";

	__asm__ __volatile__(
		"mov x8, #64\n\t"        /* SYS_WRITE */
		"mov x0, #1\n\t"         /* stdout */
		"mov x1, %0\n\t"         /* buf */
		"mov x2, %1\n\t"         /* count */
		"svc #0"
		:
		: "r" (direct), "r" ((long)(sizeof(direct) - 1))
		: "x0", "x1", "x2", "x8", "memory"
		);

	printf("\n=== All syscall tests passed! ===\n");
	return 0;
}
