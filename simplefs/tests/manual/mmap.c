#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>

#define FILE_PATH "/mnt/simplefs/mmap4"
#define FILE_SIZE 4096

int main(int argc, char **argv)
{
	const char *path = argc > 1 ? argv[1] : FILE_PATH;
	const char *msg = "Hello from mmap write!";
	char check[FILE_SIZE] = { 0 };
	char *addr;
	int fd;

	fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);
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
	addr = mmap(NULL, FILE_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		perror("mmap");
		close(fd);
		return 1;
	}

	// mmap 写入
	strcpy(addr, msg);
	if (msync(addr, FILE_SIZE, MS_SYNC) < 0) {
		perror("msync");
		return 1;
	}
	if (munmap(addr, FILE_SIZE) < 0) {
		perror("munmap");
		return 1;
	}
	if (pread(fd, check, sizeof(check), 0) < 0) {
		perror("pread");
		return 1;
	}
	close(fd);

	if (strcmp(check, msg) != 0) {
		fprintf(stderr, "mmap readback mismatch\n");
		return 1;
	}
	printf("PASS: %s\n", path);
	return 0;
}
