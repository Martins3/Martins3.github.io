#include "internal.h"
#include <linux/anon_inodes.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/fdtable.h>
#include <linux/module.h>

// 测试用的 file_operations
static ssize_t anon_inode_test_read(struct file *file, char __user *buf,
				    size_t count, loff_t *pos)
{
	pr_info("[anon_inode] read called, private_data=%p\n", file->private_data);
	return 0;
}

static ssize_t anon_inode_test_write(struct file *file, const char __user *buf,
				     size_t count, loff_t *pos)
{
	pr_info("[anon_inode] write called, private_data=%p\n", file->private_data);
	return count;
}

static const struct file_operations anon_inode_test_fops = {
	.owner = THIS_MODULE,
	.read = anon_inode_test_read,
	.write = anon_inode_test_write,
};

// 测试1: 使用 anon_inode_getfd 创建 fd
static int test_anon_inode_getfd(void)
{
	int fd;
	void *private_data = (void *)0x12345678;

	pr_info("[anon_inode] test1: anon_inode_getfd\n");

	fd = anon_inode_getfd("test_fd", &anon_inode_test_fops, private_data,
			      O_RDWR);
	if (fd < 0) {
		pr_err("[anon_inode] anon_inode_getfd failed: %d\n", fd);
		return fd;
	}

	pr_info("[anon_inode] created fd=%d, private_data=%p\n", fd,
		private_data);

	// 关闭 fd
	close_fd(fd);
	pr_info("[anon_inode] fd closed\n");
	return 0;
}

// 测试2: 使用 anon_inode_getfile 创建 file 结构
static int test_anon_inode_getfile(void)
{
	struct file *file;
	void *private_data = (void *)0xDEADBEEF;

	pr_info("[anon_inode] test2: anon_inode_getfile\n");

	file = anon_inode_getfile("test_file", &anon_inode_test_fops,
				  private_data, O_RDWR);
	if (IS_ERR(file)) {
		pr_err("[anon_inode] anon_inode_getfile failed: %ld\n",
			PTR_ERR(file));
		return PTR_ERR(file);
	}

	pr_info("[anon_inode] created file=%p, private_data=%p, inode=%p\n",
		file, file->private_data, file->f_inode);

	// 释放 file
	fput(file);
	pr_info("[anon_inode] file released\n");
	return 0;
}

// 测试3: 使用 anon_inode_create_getfile 创建独立 inode
static int test_anon_inode_create_getfile(void)
{
	struct file *file;
	void *private_data = (void *)0xAABBCCDD;

	pr_info("[anon_inode] test3: anon_inode_create_getfile\n");

	file = anon_inode_create_getfile("test_unique", &anon_inode_test_fops,
					 private_data, O_RDWR, NULL);
	if (IS_ERR(file)) {
		pr_err("[anon_inode] anon_inode_create_getfile failed: %ld\n",
			PTR_ERR(file));
		return PTR_ERR(file);
	}

	pr_info("[anon_inode] created unique file=%p, private_data=%p, inode=%p\n",
		file, file->private_data, file->f_inode);

	// 释放 file
	fput(file);
	pr_info("[anon_inode] unique file released\n");
	return 0;
}

// 测试4: 验证多个 fd 共享同一 inode
static int test_shared_inode(void)
{
	int fd1, fd2;
	struct file *file1, *file2;

	pr_info("[anon_inode] test4: verify shared inode\n");

	fd1 = anon_inode_getfd("shared_test1", &anon_inode_test_fops,
			       (void *)0x1111, O_RDWR);
	fd2 = anon_inode_getfd("shared_test2", &anon_inode_test_fops,
			       (void *)0x2222, O_RDWR);

	if (fd1 < 0 || fd2 < 0) {
		pr_err("[anon_inode] failed to create fds\n");
		if (fd1 >= 0)
			close_fd(fd1);
		if (fd2 >= 0)
			close_fd(fd2);
		return -EINVAL;
	}

	file1 = fget(fd1);
	file2 = fget(fd2);

	pr_info("[anon_inode] fd1=%d, inode1=%p, i_ino=%lu\n", fd1,
		file1->f_inode, file1->f_inode->i_ino);
	pr_info("[anon_inode] fd2=%d, inode2=%p, i_ino=%lu\n", fd2,
		file2->f_inode, file2->f_inode->i_ino);

	if (file1->f_inode == file2->f_inode) {
		pr_info("[anon_inode] SUCCESS: fd1 and fd2 share the same inode\n");
	} else {
		pr_info("[anon_inode] INFO: fd1 and fd2 have different inodes\n");
	}

	fput(file1);
	fput(file2);
	close_fd(fd1);
	close_fd(fd2);

	return 0;
}

// 测试5: 独立 inode 不共享
static int test_unique_inode(void)
{
	struct file *file1, *file2;

	pr_info("[anon_inode] test5: verify unique inode\n");

	file1 = anon_inode_create_getfile("unique_test1", &anon_inode_test_fops,
					  (void *)0x3333, O_RDWR, NULL);
	file2 = anon_inode_create_getfile("unique_test2", &anon_inode_test_fops,
					  (void *)0x4444, O_RDWR, NULL);

	if (IS_ERR(file1) || IS_ERR(file2)) {
		pr_err("[anon_inode] failed to create unique files\n");
		if (!IS_ERR(file1))
			fput(file1);
		if (!IS_ERR(file2))
			fput(file2);
		return -EINVAL;
	}

	pr_info("[anon_inode] unique file1=%p, inode1=%p, i_ino=%lu\n", file1,
		file1->f_inode, file1->f_inode->i_ino);
	pr_info("[anon_inode] unique file2=%p, inode2=%p, i_ino=%lu\n", file2,
		file2->f_inode, file2->f_inode->i_ino);

	if (file1->f_inode != file2->f_inode) {
		pr_info("[anon_inode] SUCCESS: unique files have different inodes\n");
	} else {
		pr_err("[anon_inode] ERROR: unique files share the same inode\n");
	}

	fput(file1);
	fput(file2);

	return 0;
}

int test_anon_inode(long action)
{
	int ret = 0;

	switch (action) {
	case 1:
		ret = test_anon_inode_getfd();
		break;
	case 2:
		ret = test_anon_inode_getfile();
		break;
	case 3:
		ret = test_anon_inode_create_getfile();
		break;
	case 4:
		ret = test_shared_inode();
		break;
	case 5:
		ret = test_unique_inode();
		break;
	case 0:
		// 运行所有测试
		pr_info("[anon_inode] running all tests\n");
		test_anon_inode_getfd();
		test_anon_inode_getfile();
		test_anon_inode_create_getfile();
		test_shared_inode();
		test_unique_inode();
		pr_info("[anon_inode] all tests completed\n");
		break;
	default:
		pr_info("[anon_inode] unknown action %ld\n", action);
		pr_info("[anon_inode] usage: echo [0-5] > /sys/kernel/hacking/anon_inode\n");
		pr_info("[anon_inode]  1: test anon_inode_getfd\n");
		pr_info("[anon_inode]  2: test anon_inode_getfile\n");
		pr_info("[anon_inode]  3: test anon_inode_create_getfile\n");
		pr_info("[anon_inode]  4: test shared inode\n");
		pr_info("[anon_inode]  5: test unique inode\n");
		pr_info("[anon_inode]  0: run all tests\n");
		ret = -EINVAL;
	}

	return ret;
}
