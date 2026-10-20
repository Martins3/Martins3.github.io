#define _GNU_SOURCE
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void check(int ok, const char *what)
{
	if (!ok) {
		perror(what);
		exit(1);
	}
}

static void extent_test(int create)
{
	int fd = open("extents", O_RDWR | (create ? O_CREAT | O_EXCL : 0), 0600);
	struct stat st;
	char data[8192];

	check(fd >= 0, "open extents");
	if (create) {
		for (int i = 0; i < 400; i++) {
			char value = (i % 251) + 1;
			check(pwrite(fd, &value, 1, (off_t)i * 1048576) == 1, "pwrite extent");
		}
		check(fsync(fd) == 0, "fsync extents");
	} else {
		for (int i = 0; i < 400; i++) {
			ssize_t n = pread(fd, data, sizeof(data), (off_t)i * 1048576);
			check(n == (i == 399 ? 1 : (ssize_t)sizeof(data)), "extent size");
			check((unsigned char)data[0] == (i % 251) + 1, "extent value");
			for (int j = 1; j < n; j++)
				check(data[j] == 0, "extent hole");
		}
		check(ftruncate(fd, 8192) == 0, "truncate extents");
		check(fsync(fd) == 0, "fsync truncate");
		check(fstat(fd, &st) == 0, "stat extents");
		/* One root + one data block, with no external extent leaf. */
		check(st.st_blocks == 16, "collapsed extent accounting");
	}
	check(close(fd) == 0, "close extents");
}

int main(int argc, char **argv)
{
	check(argc == 4, "usage: large-directory.out DIR COUNT create|verify|mutate|clean");
	unsigned count = strtoul(argv[2], NULL, 10);
	const char *mode = argv[3];
	if (!strcmp(mode, "ext-create")) {
		check(chdir(argv[1]) == 0, "chdir");
		extent_test(1);
		return 0;
	}
	int create = !strcmp(mode, "create");
	int verify = !strcmp(mode, "verify");
	int mutate = !strcmp(mode, "mutate");
	int clean = !strcmp(mode, "clean");
	check(create || verify || mutate || clean, "mode");
	check(chdir(argv[1]) == 0, "chdir");
	if (create) {
		check(mkdir("many", 0700) == 0, "mkdir many");
		extent_test(1);
	}
	if (verify)
		extent_test(0);
	check(chdir("many") == 0, "chdir many");
	for (unsigned i = 0; i < count; i++) {
		char name[32], value[32], data[32];
		snprintf(name, sizeof(name), "file-%08u", i);
		int len = snprintf(value, sizeof(value), "%08u", i);
		if (create || verify) {
			int fd = open(name, create ? O_CREAT | O_EXCL | O_WRONLY : O_RDONLY, 0600);
			check(fd >= 0, "open file");
			if (create)
				check(write(fd, value, len) == len, "write file");
			else {
				check(read(fd, data, sizeof(data)) == len, "read file");
				check(!memcmp(data, value, len), "file contents");
			}
			check(close(fd) == 0, "close file");
		} else if (mutate && i % 3 == 0) {
			check(unlink(name) == 0, "unlink slot");
			check(link("file-00000001", name) == 0, "reuse slot via link");
		} else if (clean) {
			check(unlink(name) == 0, "unlink all");
		}
	}
	if (mutate) {
		check(rename("file-00000001", "renamed") == 0, "rename");
		check(renameat2(AT_FDCWD, "renamed", AT_FDCWD, "file-00000002", RENAME_EXCHANGE) == 0, "exchange");
		check(renameat2(AT_FDCWD, "renamed", AT_FDCWD, "file-00000001", RENAME_EXCHANGE) < 0 && errno == ENOENT, "missing exchange");
		check(rename("renamed", "file-00000001") == 0, "rename back");
	}
	DIR *dir = opendir(".");
	check(dir != NULL, "opendir");
	unsigned actual = 0;
	struct dirent *de;
	errno = 0;
	while ((de = readdir(dir)))
		if (strcmp(de->d_name, ".") && strcmp(de->d_name, ".."))
			actual++;
	check(errno == 0, "readdir");
	check(actual == (clean ? 0 : count), "directory count");
	closedir(dir);
	if (clean) {
		check(chdir("..") == 0, "chdir parent");
		check(rmdir("many") == 0, "rmdir");
		check(unlink("extents") == 0, "unlink extents");
	}
	printf("%s: %u entries passed\n", mode, count);
	return 0;
}
