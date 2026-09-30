#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/*
 * Linux demo；编译、运行方法和预期结果见 unlink-remove.md。
 * 所有操作都在 mkdtemp() 创建的私有目录中执行，不接受外部路径。
 */
static void check(int result, const char *what)
{
	if (result == -1) {
		perror(what);
		exit(EXIT_FAILURE);
	}
}

static void verify(int condition, const char *what)
{
	if (!condition) {
		fprintf(stderr, "验证失败：%s\n", what);
		exit(EXIT_FAILURE);
	}
}

static void create_file(const char *path)
{
	int fd = open(path, O_CREAT | O_EXCL | O_WRONLY, 0600);

	check(fd, "create file");
	check(close(fd), "close");
}

static void expect_absent(const char *path)
{
	struct stat st;
	int ret = lstat(path, &st);
	int error = errno;

	verify(ret == -1 && error == ENOENT, "目录项已消失");
}

/* errno 只在调用失败时有意义；必须在其他库调用之前保存。 */
static void try_delete(const char *name, int (*delete_path)(const char *),
		       const char *path, int expected_errno)
{
	int ret = delete_path(path);
	int error = ret == -1 ? errno : 0;

	printf("  %s(\"%s\") = %d", name, path, ret);
	if (ret == -1)
		printf(", errno=%d (%s)", error, strerror(error));
	putchar('\n');
	verify(ret == (expected_errno ? -1 : 0) && error == expected_errno,
	       "删除结果符合预期");
	if (!expected_errno)
		expect_absent(path);
}

static void run_cases(const char *name, int (*delete_path)(const char *),
		      int supports_directory)
{
	static const char content[] = "hello after deletion";
	char buffer[sizeof(content)] = { 0 };
	struct stat st;
	ssize_t size;
	int fd;

	printf("\n=== %s() ===\n", name);
	puts("[普通文件：删除名字后，已打开的 fd 仍可读取]");
	fd = open("file", O_CREAT | O_EXCL | O_RDWR, 0600);
	check(fd, "open file");
	size = write(fd, content, sizeof(content) - 1);
	verify(size == (ssize_t)(sizeof(content) - 1), "写入完整内容");
	try_delete(name, delete_path, "file", 0);
	check(fstat(fd, &st), "fstat");
	verify(st.st_nlink == 0, "最后一个硬链接已删除");
	size = pread(fd, buffer, sizeof(buffer) - 1, 0);
	verify(size == (ssize_t)(sizeof(content) - 1) &&
		       memcmp(buffer, content, sizeof(content) - 1) == 0,
	       "删除后仍能通过 fd 读取原内容");
	printf("  路径已消失，st_nlink=%lu，fd 读到：%s\n",
	       (unsigned long)st.st_nlink, buffer);
	check(close(fd), "close");

	puts("[空目录]");
	check(mkdir("empty-dir", 0700), "mkdir");
	try_delete(name, delete_path, "empty-dir",
		   supports_directory ? 0 : EISDIR);
	if (!supports_directory)
		check(rmdir("empty-dir"), "cleanup empty directory");

	puts("[非空目录：两者都不会递归删除]");
	check(mkdir("full-dir", 0700), "mkdir");
	create_file("full-dir/child");
	try_delete(name, delete_path, "full-dir",
		   supports_directory ? ENOTEMPTY : EISDIR);
	check(lstat("full-dir/child", &st), "child still exists");
	check(unlink("full-dir/child"), "cleanup child");
	check(rmdir("full-dir"), "cleanup full directory");

	puts("[指向目录的符号链接：仅删除链接本身]");
	check(mkdir("target-dir", 0700), "mkdir");
	create_file("target-dir/child");
	check(symlink("target-dir", "dir-link"), "symlink");
	try_delete(name, delete_path, "dir-link", 0);
	check(lstat("target-dir/child", &st), "symlink target still exists");
	puts("  target-dir/child 仍然存在");
	check(unlink("target-dir/child"), "cleanup target child");
	check(rmdir("target-dir"), "cleanup target directory");

	puts("[不存在的路径]");
	try_delete(name, delete_path, "missing", ENOENT);
}

int main(void)
{
	char workspace[] = "/tmp/unlink-remove-demo-XXXXXX";

	if (mkdtemp(workspace) == NULL) {
		perror("mkdtemp");
		return EXIT_FAILURE;
	}
	printf("临时目录：%s\n", workspace);
	check(chdir(workspace), "chdir");
	run_cases("unlink", unlink, 0);
	run_cases("remove", remove, 1);
	check(chdir("/"), "leave temporary directory");
	check(rmdir(workspace), "cleanup temporary directory");
	puts("\n全部验证通过，临时目录已清理。");
	return EXIT_SUCCESS;
}
