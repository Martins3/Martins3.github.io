/*
 * 参考 drivers/char/random.c 的实现
 */
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/cdev.h>
#include <linux/spinlock.h>
#include <linux/completion.h>
#include <linux/poll.h>
#include "internal.h"

static int dev_major = 0;
static DECLARE_WAIT_QUEUE_HEAD(amsg_init_wait);
static int buffer_ready = -1;

#define BUFFER_LEN 32
static char BUFFER[BUFFER_LEN];

static ssize_t amsg_read(struct file *file, char __user *buf, size_t len,
			 loff_t *ppos)
{
	ssize_t count = 0;
	pr_info("[%s:%s] \n", __FUNCTION__, current->comm);
	count = simple_read_from_buffer(buf, len, ppos, BUFFER, BUFFER_LEN);
	if (count < 0)
		return count;
	buffer_ready = 0;
	wake_up_interruptible(&amsg_init_wait);
	return count;
}
static ssize_t amsg_write(struct file *file, const char __user *user_buf,
			  size_t count, loff_t *ppos)
{
	ssize_t r = 0;
	pr_info("[%s:%d] \n", __FUNCTION__, __LINE__);
	r = simple_write_to_buffer(BUFFER, BUFFER_LEN, ppos, user_buf, count);
	if (r < 0)
		return r;
	buffer_ready = 1;
	wake_up_interruptible(&amsg_init_wait);
	return r;
}

/*
 *
 * epoll wait 会等待所有 event ，所以这里的 poll_wait 不是阻塞的，其作用只是将
 * wait 加入到 wait queue 中而已。实际的等待位置在 fs/eventpoll.c:ep_poll
 *
 *  amsg_poll
 *  ep_item_poll.isra.0
 *  do_epoll_wait
 *  __x64_sys_epoll_wait
 *  do_syscall_64
 *  entry_SYSCALL_64_after_hwframe
 *
 * amsg_poll 的返回值的含义是 : 当调用 epoll_wait 的时候系统的状态。
 */
static __poll_t amsg_poll(struct file *file, poll_table *wait)
{
	/*
	 * 通过 poll_wait 来将 poll_table 中的 entry 添加到 amsg_init_wait 这个 waitqueue 中
	 * 如果是使用 epoll 来监听，那么这个 wait->_qproc 就是 ep_ptable_queue_proc 了
	 */
	pr_info("[martins3:%s:%d] %px\n", __func__, __LINE__, wait->_qproc);
	poll_wait(file, &amsg_init_wait, wait);
	if (buffer_ready == -1)
		return 0;
	return buffer_ready ? EPOLLIN : EPOLLOUT;
}

/* This is called whenever a process attempts to open the device file */
static int device_open(struct inode *inode, struct file *file)
{
	pr_info("device_open\n");
	try_module_get(THIS_MODULE);
	return 0;
}

static int device_release(struct inode *inode, struct file *file)
{
	pr_info("device_release\n");
	module_put(THIS_MODULE);
	return 0;
}

// TODO 不要注册 device_open 函数会如何?
static struct file_operations amsg_fops = { .owner = THIS_MODULE,
					    .open = device_open,
					    .read = amsg_read,
					    .write = amsg_write,
					    .release = device_release,
					    .poll = amsg_poll };

static struct class *cls;
#define DEVICE_FILE_NAME "amsg"

int test_epoll_init(void)
{
	dev_major = register_chrdev(dev_major, DEVICE_FILE_NAME, &amsg_fops);
	if (dev_major < 0) {
		pr_info("Unable to register\n");
		return dev_major;
	}
	cls = class_create(DEVICE_FILE_NAME);
	device_create(cls, NULL, MKDEV(dev_major, 0), NULL, DEVICE_FILE_NAME);
	pr_info("amsg major=%d\n", dev_major);
	return 0;
}

int test_epoll_exit(void)
{
	device_destroy(cls, MKDEV(dev_major, 0));
	class_destroy(cls);
	unregister_chrdev(dev_major, DEVICE_FILE_NAME);
	pr_info("clean up amsg\n");
	return 0;
}

int test_epoll(long action)
{
	return 0;
}
