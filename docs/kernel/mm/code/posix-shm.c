/*
 * 验证一个结论:
 * 在 Linux 上，shm_open("/foo") 通常对应 /dev/shm/foo，设备号和 inode 相同。
 * shm_unlink("/foo") 会删除该名字，但已有 fd 和 mmap 映射继续有效，直到最后 close() 和 munmap()。
 */
#define _POSIX_C_SOURCE 200809L

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define SHM_SIZE 4096

static void die(const char *what)
{
	perror(what);
	exit(EXIT_FAILURE);
}

static void show_fd_link(int fd)
{
	char fd_path[64];
	char target[256];
	ssize_t len;

	if (snprintf(fd_path, sizeof(fd_path), "/proc/self/fd/%d", fd) < 0)
		die("snprintf");

	len = readlink(fd_path, target, sizeof(target) - 1);
	if (len == -1)
		die("readlink");
	target[len] = '\0';
	printf("  %s -> %s\n", fd_path, target);
}

static void child_open_and_update(const char *name, int inherited_fd,
				  char *inherited_mapping)
{
	char *mapping;
	int fd;

	/* Force the child to look the object up by name instead of using forked refs. */
	if (munmap(inherited_mapping, SHM_SIZE) == -1)
		die("child munmap");
	if (close(inherited_fd) == -1)
		die("child close");

	fd = shm_open(name, O_RDWR, 0);
	if (fd == -1)
		die("child shm_open");
	mapping = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mapping == MAP_FAILED)
		die("child mmap");

	printf("child: shm_open(\"%s\") reads: %s\n", name, mapping);
	if (snprintf(mapping, SHM_SIZE, "updated by child pid %ld", (long)getpid()) < 0)
		die("child snprintf");

	if (munmap(mapping, SHM_SIZE) == -1)
		die("child munmap");
	if (close(fd) == -1)
		die("child close");
	exit(EXIT_SUCCESS);
}

int main(void)
{
	char name[64];
	char path[128];
	char buffer[SHM_SIZE];
	char *mapping;
	struct stat fd_stat;
	struct stat path_stat;
	pid_t child;
	int status;
	int fd;

	if (snprintf(name, sizeof(name), "/posix-shm-demo-%ld", (long)getpid()) < 0)
		die("snprintf name");
	if (snprintf(path, sizeof(path), "/dev/shm%s", name) < 0)
		die("snprintf path");

	fd = shm_open(name, O_CREAT | O_EXCL | O_RDWR, 0600);
	if (fd == -1)
		die("shm_open");
	if (ftruncate(fd, SHM_SIZE) == -1)
		die("ftruncate");
	mapping = mmap(NULL, SHM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
	if (mapping == MAP_FAILED)
		die("mmap");

	if (fstat(fd, &fd_stat) == -1)
		die("fstat");
	if (stat(path, &path_stat) == -1)
		die("stat /dev/shm entry");

	printf("created:\n");
	printf("  POSIX name: %s\n", name);
	printf("  Linux path: %s\n", path);
	printf("  fd dev:inode   = %ju:%ju\n", (uintmax_t)fd_stat.st_dev,
	       (uintmax_t)fd_stat.st_ino);
	printf("  path dev:inode = %ju:%ju\n", (uintmax_t)path_stat.st_dev,
	       (uintmax_t)path_stat.st_ino);
	show_fd_link(fd);

	if (snprintf(mapping, SHM_SIZE, "written by parent pid %ld", (long)getpid()) < 0)
		die("snprintf mapping");
	if (fflush(stdout) == EOF)
		die("fflush");

	child = fork();
	if (child == -1)
		die("fork");
	if (child == 0)
		child_open_and_update(name, fd, mapping);
	if (waitpid(child, &status, 0) == -1)
		die("waitpid");
	if (!WIFEXITED(status) || WEXITSTATUS(status) != EXIT_SUCCESS) {
		fprintf(stderr, "child failed\n");
		exit(EXIT_FAILURE);
	}
	printf("parent: shared mapping now reads: %s\n", mapping);

	if (shm_unlink(name) == -1)
		die("shm_unlink");
	printf("\nafter shm_unlink(\"%s\"):\n", name);
	if (stat(path, &path_stat) == -1 && errno == ENOENT)
		printf("  stat(%s): ENOENT (the /dev/shm name is gone)\n", path);
	else {
		fprintf(stderr, "unexpected: %s still exists or stat failed differently\n", path);
		exit(EXIT_FAILURE);
	}
	show_fd_link(fd);

	errno = 0;
	status = shm_open(name, O_RDWR, 0);
	if (status == -1 && errno == ENOENT)
		printf("  a new shm_open by name: ENOENT\n");
	else {
		if (status != -1)
			close(status);
		fprintf(stderr, "unexpected: shm_open found the unlinked name\n");
		exit(EXIT_FAILURE);
	}

	if (snprintf(mapping, SHM_SIZE, "still alive after shm_unlink") < 0)
		die("snprintf mapping");
	memset(buffer, 0, sizeof(buffer));
	if (pread(fd, buffer, sizeof(buffer) - 1, 0) == -1)
		die("pread after shm_unlink");
	printf("  existing mapping/fd still read: %s\n", buffer);

	if (munmap(mapping, SHM_SIZE) == -1)
		die("munmap");
	if (close(fd) == -1)
		die("close");
	printf("\nafter munmap + close: the last references are released\n");
	return EXIT_SUCCESS;
}
