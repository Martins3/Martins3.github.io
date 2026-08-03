#include "internal.h"
#include <linux/debugfs.h>

/**
 * TODO 真的还需要一个 seq fops ，好麻烦啊
 * https://docs.kernel.org/translations/zh_CN/filesystems/debugfs.html
 */

static struct dentry *debugfs_dir;

static int local_show(struct seq_file *m, void *v)
{
	seq_printf(m, "ab\ncd\n");
	return 0;
}

static int local_open(struct inode *inode, struct file *file)
{
	return single_open(file, local_show, NULL);
}

static const struct file_operations fops = {
	.llseek = seq_lseek,
	.open = local_open,
	.owner = THIS_MODULE,
	.read = seq_read,
	.release = single_release,
};

static void create(int id)
{
	char name[20];
	if (debugfs_dir)
		return;

	snprintf(name, sizeof(name), "martins3-%u", id);
	debugfs_dir = debugfs_create_dir(name, debugfs_dir);
	debugfs_create_file("mmu_rmaps_stat", 0644, debugfs_dir, NULL, &fops);
}

#define len 200
u64 intvalue, hexvalue;
struct dentry *dirret, *fileret, *u64int, *u64hex;
char ker_buf[len];
int filevalue;
/* read file operation */
static ssize_t myreader(struct file *fp, char __user *user_buffer, size_t count,
			loff_t *position)
{
	return simple_read_from_buffer(user_buffer, count, position, ker_buf,
				       len);
}

/* write file operation */
static ssize_t mywriter(struct file *fp, const char __user *user_buffer,
			size_t count, loff_t *position)
{
	if (count > len)
		return -EINVAL;

	return simple_write_to_buffer(ker_buf, len, position, user_buffer,
				      count);
}

static const struct file_operations fops_debug = {
	.read = myreader,
	.write = mywriter,
};

static int init_debug(void)
{
	/* create a directory by the name dell in /sys/kernel/debugfs */
	dirret = debugfs_create_dir("dell", NULL);

	/* create a file in the above directory
    This requires read and write file operations */
	fileret = debugfs_create_file("text", 0644, dirret, &filevalue,
				      &fops_debug);

	/* create a file which takes in a int(64) value */
	debugfs_create_u64("number", 0644, dirret, &intvalue);
	if (!u64int) {
		printk("error creating int file");
		return (-ENODEV);
	}
	/* takes a hex decimal value */
	debugfs_create_x64("hexnum", 0644, dirret, &hexvalue);
	if (!u64hex) {
		printk("error creating hex file");
		return (-ENODEV);
	}

	return (0);
}

static void exit_debug(void)
{
	debugfs_remove_recursive(dirret);
}

int test_debugfs(long action)
{
	switch (action) {
	case 1:
		create(action);
		break;
	case 2:
		debugfs_remove(debugfs_dir);
		break;
	case 3:
		// TODO 到时候检查看看这里的 api 
		init_debug();
		break;
	case 4:
		exit_debug();
	}
	return 0;
}
