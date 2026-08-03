#include "internal.h"
#include <linux/miscdevice.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/percpu-refcount.h>
#include <uapi/linux/kvm.h>

// TODO compat_ioctl 和 ioctl 啥关系 ?
//
// get_user 和 put_user 几乎总是和 ioctl 和 syscall 放到一起，这是无法避免的
#define KVM_COMPAT(c) .compat_ioctl = (c)

/**
 * https://docs.kernel.org/driver-api/ioctl.html
 * https://docs.kernel.org/userspace-api/ioctl/ioctl-number.html
 *
 * 实现参考: 但是比较容易
 * https://github.com/sysprog21/lkmpg/blob/master/examples/ioctl.c
 *
 * 参考 kvm_dev_ioctl 的实现
 */
static long dev_ioctl(struct file *filp, unsigned int ioctl, unsigned long arg)
{
	int r = -EINVAL;

	switch (ioctl) {
	/* case KVM_GET_API_VERSION: */
	/* 	break; */
	default:
		break;
	}
	return r;
}

#define BUF_SIZE 24
static char msg_buf[24] = "martins3\n";
static ssize_t amsg_read(struct file *file, char __user *buf, size_t len,
			 loff_t *ppos)
{
	return simple_read_from_buffer(buf, len, ppos, msg_buf, BUF_SIZE);
}

static struct file_operations kvm_chardev_ops = {
	.unlocked_ioctl = dev_ioctl,
	.llseek = noop_llseek,
	.read = amsg_read,
	KVM_COMPAT(dev_ioctl),
};

// 用这个测试下 mknod
static struct miscdevice kvm_dev = {
	0,
	"martins3_misc_dev",
	&kvm_chardev_ops,
};

static struct class *cls = (struct class *)1;
#define DEVICE_FILE_NAME "char2"
static int dev_major;
static struct file_operations amsg_fops = {
	.owner = THIS_MODULE,
};

// 没想到，这种使用一个 class 的方法就是非常 general 方法
static int register_general_char(void)
{
	dev_major = register_chrdev(dev_major, DEVICE_FILE_NAME, &amsg_fops);
	if (dev_major < 0) {
		pr_info("Unable to register\n");
		return dev_major;
	}
	pr_info("amsg major=%d\n", dev_major);
	cls = class_create(DEVICE_FILE_NAME);
	device_create(cls, NULL, MKDEV(dev_major, 0), NULL, DEVICE_FILE_NAME);
	return 0;
}
#define MARTIN_MINORS (1U << 2)
static dev_t ublk_chr_devt;
static struct device dev;
static struct cdev cdev;
static int register_raw(void)
{
	int minor = 0;
	int ret = alloc_chrdev_region(&ublk_chr_devt, 0, MARTIN_MINORS,
				      "ublk-char");
	if (ret)
		return ret;

	dev.devt = MKDEV(MAJOR(ublk_chr_devt), minor);
	dev.class = cls;
	device_initialize(&dev);

	ret = dev_set_name(&dev, "ublkc%d", minor);
	if (ret)
		return ret;
	cdev_init(&cdev, &amsg_fops);
	ret = cdev_device_add(&cdev, &dev);
	if (ret)
		return ret;
	return 0;
}

// 参考 tty_init 写的方法
static struct cdev tty_cdev;
#define MY_TTYAUX_MAJOR 18
static int simple_way(void)
{
	cdev_init(&tty_cdev, &amsg_fops);
	if (cdev_add(&tty_cdev, MKDEV(MY_TTYAUX_MAJOR, 0), 1)) {
		panic("cdev add\n");
	}
	if (register_chrdev_region(MKDEV(MY_TTYAUX_MAJOR, 0), 1,
				   "/dev/my_tty") < 0)
		panic("Couldn't register /dev/tty driver\n");
	device_create(cls, NULL, MKDEV(MY_TTYAUX_MAJOR, 0), NULL, "my_tty");
	return 0;
}

int test_dev_init(void)
{
	int err = misc_register(&kvm_dev);
	if (err)
		return err;

	err = register_general_char();
	if (err)
		return err;

	err = register_raw();
	if (err)
		return err;

	// TODO 真的还是懵懵的，没搞懂为什么 cdev 的基本使用方法
	// 不理解为什么总是需要和 class 在一起使用啊
	err = simple_way();
	if (err)
		return err;

	return 0;
}

//  TODO 这里有 bug ，不可以重新加载的
int test_dev_exit(void)
{
	misc_deregister(&kvm_dev);
	class_unregister(cls);
	unregister_chrdev_region(dev_major, 1);

	return 0;
}

int test_dev(long action)
{
	// 具体测试参考 ./dev.sh
	switch (action) {
	case 1:
		// 输出内容如下:
		// misc martins3_misc_dev: dev info
		// misc martins3_misc_dev: dev notice
		pr_info("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
		dev_info(kvm_dev.this_device, "dev info\n");
		dev_notice(kvm_dev.this_device, "dev notice\n");
		dev_dbg_ratelimited(kvm_dev.this_device, "dev_dbg\n");
		break;
	case 2:
		// TODO 先感受一下 ioctl 字段是什么
		pr_info("[martins3:%s:%d] %lx\n", __FUNCTION__, __LINE__, KVM_SET_MSRS);
		break;
	}

	return 0;
}
