// 参考 https://github.com/iluqian/netlink-demo
// 内核 netlink 接口示例
#include "internal.h"
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <net/sock.h>

#define NETLINK_USER 22
#define USER_MSG (NETLINK_USER + 1)
#define USER_PORT 50

static struct sock *netlinkfd = NULL;

// 向用户空间发送消息
static int send_msg(int8_t *pbuf, uint16_t len)
{
	struct sk_buff *nl_skb;
	struct nlmsghdr *nlh;
	int ret;

	pr_debug("Entering %s, line=%d, len=%d\n", __FUNCTION__, __LINE__, len);

	nl_skb = nlmsg_new(len, GFP_KERNEL);
	if (!nl_skb) {
		pr_err("netlink_alloc_skb error\n");
		return -ENOMEM;
	}

	pr_debug("skb_tailroom(nl_skb)=%u, nlmsg_total_size(USER_MSG)=%u\n",
		 skb_tailroom(nl_skb), nlmsg_total_size(USER_MSG));

	nlh = __nlmsg_put(nl_skb, 0, 0, USER_MSG, len, 0);
	if (nlh == NULL) {
		pr_err("nlmsg_put() error\n");
		nlmsg_free(nl_skb);
		return -EMSGSIZE;
	}

	memcpy(nlmsg_data(nlh), pbuf, len);

	ret = netlink_unicast(netlinkfd, nl_skb, USER_PORT, MSG_DONTWAIT);
	if (ret < 0) {
		pr_err("netlink_unicast failed: %d\n", ret);
		return ret;
	}

	return 0;
}

// 接收用户空间消息的回调函数
static void recv_cb(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = NULL;
	void *data = NULL;

	pr_debug("Entering %s, line=%d\n", __FUNCTION__, __LINE__);
	pr_debug("skb->len:%u\n", skb->len);

	// 打印接收到的数据包内容（调试用）
	if (skb) {
		char *buf = skb->data;
		int len = skb->len;
		int i;
		pr_debug("[%s:%d] Packet length = %#4x\n", __FUNCTION__,
			 __LINE__, len);
		for (i = 0; i < len; i++) {
			if (i % 16 == 0)
				pr_debug("%#4.4x", i);
			if (i % 2 == 0)
				pr_debug(" ");
			pr_debug("%2.2x", ((unsigned char *)buf)[i]);
			if (i % 16 == 15)
				pr_debug("\n");
		}
		pr_debug("\n\n");
	}

	if (skb->len >= nlmsg_total_size(0)) {
		nlh = nlmsg_hdr(skb);
		data = NLMSG_DATA(nlh);
		if (data) {
			pr_info("kernel receive data: %s\n", (int8_t *)data);
			pr_debug("nlmsg_len(nlh)= %d\n", nlmsg_len(nlh));

			// 将接收到的数据回传给用户空间
			send_msg(data, nlmsg_len(nlh));
		}
	}
}

static struct netlink_kernel_cfg cfg = {
	.input = recv_cb,
};

// 模块退出时清理资源
int test_netlink_exit(void)
{
	if (netlinkfd) {
		sock_release(netlinkfd->sk_socket);
		netlinkfd = NULL;
		pr_info("netlink socket released\n");
	}
	return 0;
}

// 模块初始化时创建 netlink socket
int test_netlink_init(void)
{
	pr_info("Entering %s, line=%d\n", __FUNCTION__, __LINE__);

	netlinkfd = netlink_kernel_create(&init_net, USER_MSG, &cfg);
	if (!netlinkfd) {
		pr_err("can not create a netlink socket!\n");
		return -ENOMEM;
	}

	pr_info("netlink socket created successfully (protocol=%d, port=%d)\n",
		USER_MSG, USER_PORT);
	return 0;
}

// 测试入口函数
int test_netlink(long action)
{
	switch (action) {
	case 0:
		// action 0: 仅检查 netlink 是否已初始化
		if (netlinkfd) {
			pr_info("netlink is initialized and ready\n");
		} else {
			pr_err("netlink is not initialized\n");
			return -ENODEV;
		}
		break;
	case 1:
		// action 1: 发送测试消息到用户空间（如果用户空间程序正在监听）
		pr_info("sending test message to userspace...\n");
		{
			const char *test_msg =
				"Hello from kernel (test action 1)";
			int ret = send_msg((int8_t *)test_msg,
					   strlen(test_msg) + 1);
			if (ret < 0) {
				pr_err("failed to send test message: %d\n",
				       ret);
				return ret;
			}
			pr_info("test message sent successfully\n");
		}
		break;
	default:
		pr_err("unknown action: %ld\n", action);
		return -EINVAL;
	}
	return 0;
}
