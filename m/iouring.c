#define pr_fmt(fmt) "folio : " fmt

#include "internal.h"
#include <linux/gfp.h>
#include <linux/mm.h>
#include <asm/io.h>
#include <linux/miscdevice.h>

static void *share_mem = NULL;

static ssize_t amsg_read(struct file *file, char __user *buf, size_t len,
			 loff_t *ppos)
{
	if (!share_mem)
		return -ENOSPC;

	ssize_t count = 0;
	count = simple_read_from_buffer(buf, len, ppos, share_mem, PAGE_SIZE);
	if (count < 0)
		return count;
	return count;
}

static int martin_uring_cmd(struct io_uring_cmd *ioucmd,
			    unsigned int issue_flags)
{
	pr_info("[%s:%d] \n", __FUNCTION__, __LINE__);
	return 0;
}

static int martin_uring_cmd_iopoll(struct io_uring_cmd *ioucmd,
				   struct io_comp_batch *iob,
				   unsigned int poll_flags)
{
	pr_info("[%s:%d] \n", __FUNCTION__, __LINE__);
#if 0
#TODO 如何实现套娃的 ?
#TODO 用户态这个部分如何触发 ?
	struct nvme_uring_cmd_pdu *pdu = nvme_uring_cmd_pdu(ioucmd);
	struct request *req = pdu->req;

	if (req && blk_rq_is_poll(req))
		return blk_rq_poll(req, iob, poll_flags);
#endif
	return 0;
}

static struct file_operations kvm_chardev_ops = {
	.llseek = noop_llseek,
	.read = amsg_read,
	.uring_cmd = martin_uring_cmd,
	.uring_cmd_iopoll = martin_uring_cmd_iopoll
};

static struct miscdevice kvm_dev = {
	MISC_DYNAMIC_MINOR,
	"iouring",
	&kvm_chardev_ops,
};

static bool register_status = false;

int test_iouring(long action)
{
	// TODO 看看用啥方法，构建一个统计的方法在卸载的
	// register 一个 calling chain ?
	// 只要 echo 过，自动注册，或者其他的方法 ?
	if (!register_status) {
		pr_info("register /dev/iouring\n");
		misc_register(&kvm_dev);
		register_status = true;
	}

	switch (action) {
	case 0:
		misc_deregister(&kvm_dev);
		break;
	}
	return 0;
}
