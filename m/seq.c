#include "internal.h"
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/module.h>

static struct dentry *debugfs_file;

struct student {
	char name[100];
	int id;
};

struct student classmates[] = { { .id = 1, .name = "Alice" },
				{ .id = 2, .name = "Bob" },
				{ .id = 3, .name = "Kelvin" } };

static void *start(struct seq_file *s, loff_t *pos)
{
	pr_info("start");
	if (*pos == 0) {
		return classmates;
	}

	/**
   *
   * - 参考:
   * -
   * https://stackoverflow.com/questions/25399112/how-to-use-a-seq-file-in-linux-kernel-modules
   * - fs/proc/consoles.c
   *
   * 整个执行流程，最后会执行下:
   *
   * [   94.251027] open
   * [   94.251160] start
   * [   94.251161] show
   * [   94.251253] next
   * [   94.251338] show
   * [   94.251421] next
   * [   94.251507] show
   * [   94.251594] next
   * [   94.251680] stop
   * [   94.251856] start
   * [   94.251857] [martins3:start:40]
   * [   94.252140] stop
   *
   * 简单来说，seqfile 就是用于输出 kernel 中结构化的数据的
   *
   * 1. start 和 stop 作为开始和结束的 hook
   * 2. next 来滑动
   * 3. show 来输出
   *
   * 此外，遍历 kernel 中常用的数据结构，例如 list 和 hlist 其实已经有写
   * 好的。
   *
   * cat /sys/kernel/debug/martins3_seq 为什么会调用两次 start - stop
   * 因为每次 read syscall 其实都会走到一次 start - stop ，cat 是那种
   * while(read() > 0) 的结构
   *
   * 所以这里的 return NULL 可以让 read 返回失败。
   */
	pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	*pos = 0;
	return NULL;
}

static void *next(struct seq_file *s, void *v, loff_t *pos)
{
	struct student *spos = (struct student *)v + 1;
	*pos += sizeof(struct student);
	pr_info("next");
	if (*pos >= sizeof(classmates))
		return NULL;
	return spos;
}

static void stop(struct seq_file *s, void *v)
{
	pr_info("stop\n");
}

static int show(struct seq_file *s, void *v)
{
	struct student *spos = v;
	pr_info("show");
	seq_printf(s, "%s %d\n", spos->name, spos->id);
	return 0;
}

static struct seq_operations seq_ops = {
	.next = next,
	.show = show,
	.start = start,
	.stop = stop,
};

/*
 * test the doc
 */
static int open(struct inode *inode, struct file *file)
{
	pr_info("open\n");
	return seq_open(file, &seq_ops);
}

static struct file_operations fops = { .owner = THIS_MODULE,
				       .llseek = seq_lseek,
				       .open = open,
				       .read = seq_read,
				       .release = seq_release };

int test_seq_init(void)
{
	debugfs_file =
		debugfs_create_file("martins3_seq", S_IRUSR, NULL, NULL, &fops);
	return debugfs_file == NULL;
}

int test_seq_exit(void)
{
	debugfs_remove(debugfs_file);
	return 0;
}

int test_seq(long action)
{
	switch (action) {
		break;
	}
	return 0;
}
