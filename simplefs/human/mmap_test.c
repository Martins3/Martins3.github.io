#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#define FILE_PATH "/mnt/simplefs/mmap4"
#define FILE_SIZE 4096

int main(void)
{
	int fd = open(FILE_PATH, O_RDWR | O_CREAT, 0644);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	// 扩展文件大小
	if (ftruncate(fd, FILE_SIZE) < 0) {
		perror("ftruncate");
		close(fd);
		return 1;
	}

	// mmap 映射
	char *addr = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED,
			  fd, 0);
	if (addr == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return 1;
	}

	// mmap 写入
	const char *msg = "Hello from mmap write!";
	strcpy(addr, msg);
	printf("[WRITE] Written: %s\n", msg);
	return 0;
}
