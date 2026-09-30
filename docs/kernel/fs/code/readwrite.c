/*
 * 测试 linux 中的几个同步 io engine
 * fio --enghelp
 *       sync
 *       psync
 *       vsync
 *       pvsync
 *       pvsync2
 *       ...
 *
 * 也就是 read pread readv preadv preadv2
 *
 * TODO ds 中拷贝过来的，没有完全跑通
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define TEMP_FILE "test_syscall.tmp"

// 检查 preadv2 是否可用
#ifndef SYS_preadv2
#define SYS_preadv2 -1
#endif

void cleanup(int fd, const char *filename)
{
	if (fd >= 0)
		close(fd);
	unlink(filename);
}

int main()
{
	int fd;
	char buffer[BUFFER_SIZE];
	char write_data[] = "Hello, System Calls!";
	struct iovec iov[2];
	ssize_t ret;

	// 创建临时文件
	fd = open(TEMP_FILE, O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	// 1. write syscall
	printf("\nTesting write:\n");
	ret = write(fd, write_data, strlen(write_data));
	if (ret < 0) {
		perror("write");
		cleanup(fd, TEMP_FILE);
		return 1;
	}
	printf("Wrote %zd bytes: %s\n", ret, write_data);

	// 重新定位文件偏移量
	lseek(fd, 0, SEEK_SET);

	// 2. read syscall
	printf("\nTesting read:\n");
	memset(buffer, 0, BUFFER_SIZE);
	ret = read(fd, buffer, BUFFER_SIZE);
	if (ret < 0) {
		perror("read");
		cleanup(fd, TEMP_FILE);
		return 1;
	}
	printf("Read %zd bytes: %s\n", ret, buffer);

	// 3. pwrite syscall
	printf("\nTesting pwrite:\n");
	char pwrite_data[] = "Pwrite data";
	ret = pwrite(fd, pwrite_data, strlen(pwrite_data), 10);
	if (ret < 0) {
		perror("pwrite");
		cleanup(fd, TEMP_FILE);
		return 1;
	}
	printf("Pwrite %zd bytes at offset 10: %s\n", ret, pwrite_data);

	// 4. pread syscall
	printf("\nTesting pread:\n");
	memset(buffer, 0, BUFFER_SIZE);
	ret = pread(fd, buffer, BUFFER_SIZE, 10);
	if (ret < 0) {
		perror("pread");
		cleanup(fd, TEMP_FILE);
		return 1;
	}
	printf("Pread %zd bytes from offset 10: %s\n", ret, buffer);

	// 5. writev syscall
	printf("\nTesting writev:\n");
	iov[0].iov_base = "First ";
	iov[0].iov_len = 6;
	iov[1].iov_base = "vector";
	iov[1].iov_len = 6;

	lseek(fd, 0, SEEK_SET);
	ret = writev(fd, iov, 2);
	if (ret < 0) {
		perror("writev");
		cleanup(fd, TEMP_FILE);
		return 1;
	}
	printf("Writev %zd bytes\n", ret);

	// 6. readv syscall
	printf("\nTesting readv:\n");
	char readv_buf1[10], readv_buf2[10];
	memset(readv_buf1, 0, sizeof(readv_buf1));
	memset(readv_buf2, 0, sizeof(readv_buf2));
	iov[0].iov_base = readv_buf1;
	iov[0].iov_len = 6;
	iov[1].iov_base = readv_buf2;
	iov[1].iov_len = 6;

	lseek(fd, 0, SEEK_SET);
	ret = readv(fd, iov, 2);
	if (ret < 0) {
		perror("readv");
		cleanup(fd, TEMP_FILE);
		return 1;
	}
	printf("Readv %zd bytes: %s%s\n", ret, readv_buf1, readv_buf2);

	// 7. pwritev syscall
	printf("\nTesting pwritev:\n");
	iov[0].iov_base = "Pwritev ";
	iov[0].iov_len = 8;
	iov[1].iov_base = "data";
	iov[1].iov_len = 4;

	ret = pwritev(fd, iov, 2, 20);
	if (ret < 0) {
		perror("pwritev");
		cleanup(fd, TEMP_FILE);
		return 1;
	}
	printf("Pwritev %zd bytes at offset 20\n", ret);

	// 8. preadv syscall
	printf("\nTesting preadv:\n");
	memset(readv_buf1, 0, sizeof(readv_buf1));
	memset(readv_buf2, 0, sizeof(readv_buf2));
	ret = preadv(fd, iov, 2, 20);
	if (ret < 0) {
		perror("preadv");
		cleanup(fd, TEMP_FILE);
		return 1;
	}
	printf("Preadv %zd bytes from offset 20: %s%s\n", ret, readv_buf1,
	       readv_buf2);

	// 9. preadv2 syscall (可能不被所有系统支持)
	printf("\nTesting preadv2:\n");
	if (SYS_preadv2 != -1) {
		memset(readv_buf1, 0, 10);
		memset(readv_buf2, 0, 10);
		ret = preadv2(fd, iov, 2, 20, 0);
		if (ret < 0) {
			perror("preadv2");
		} else {
			printf("Preadv2 %zd bytes from offset 20: %s%s\n", ret,
			       readv_buf1, readv_buf2);
		}
	} else {
		printf("preadv2 not supported on this system\n");
	}

	// 清理
	cleanup(fd, TEMP_FILE);
	return 0;
}
