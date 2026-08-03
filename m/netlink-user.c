/**
 * netlink-user.c - 用户空间 netlink 测试程序
 * 
 * 用于与内核 netlink 模块通信的测试工具
 * 
 * 使用方法:
 *   ./netlink-user.out [action]
 * 
 * action:
 *   0 - 基本测试：发送消息并等待回复
 *   1 - 循环测试：连续发送多条消息
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <unistd.h>
#include <errno.h>

#define NETLINK_USER 22
#define USER_MSG (NETLINK_USER + 1)
#define USER_PORT 50
#define MAX_PAYLOAD 1024

struct nl_msg {
	struct nlmsghdr hdr;
	char data[MAX_PAYLOAD];
};

static int create_netlink_socket(void)
{
	int sockfd = socket(AF_NETLINK, SOCK_RAW, USER_MSG);
	if (sockfd < 0) {
		perror("socket");
		return -1;
	}
	return sockfd;
}

static int bind_netlink_socket(int sockfd)
{
	struct sockaddr_nl local;

	memset(&local, 0, sizeof(local));
	local.nl_family = AF_NETLINK;
	local.nl_pid = USER_PORT;
	local.nl_groups = 0;

	if (bind(sockfd, (struct sockaddr *)&local, sizeof(local)) < 0) {
		perror("bind");
		return -1;
	}
	return 0;
}

static int send_to_kernel(int sockfd, const char *msg)
{
	struct sockaddr_nl dest_addr;
	struct nlmsghdr *nlh;
	char buffer[NLMSG_SPACE(MAX_PAYLOAD)];
	int msg_len = strlen(msg) + 1;

	if (msg_len > MAX_PAYLOAD) {
		fprintf(stderr, "message too long (max %d bytes)\n",
			MAX_PAYLOAD);
		return -1;
	}

	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.nl_family = AF_NETLINK;
	dest_addr.nl_pid = 0;
	dest_addr.nl_groups = 0;

	memset(buffer, 0, sizeof(buffer));
	nlh = (struct nlmsghdr *)buffer;
	nlh->nlmsg_len = NLMSG_SPACE(msg_len);
	nlh->nlmsg_type = 0;
	nlh->nlmsg_flags = 0;
	nlh->nlmsg_seq = 0;
	nlh->nlmsg_pid = USER_PORT;

	memcpy(NLMSG_DATA(nlh), msg, msg_len);

	if (sendto(sockfd, buffer, nlh->nlmsg_len, 0,
		   (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0) {
		perror("sendto");
		return -1;
	}

	printf("sent to kernel: %s\n", msg);
	return 0;
}

static int receive_from_kernel(int sockfd, char *buffer, size_t buf_size)
{
	struct nl_msg msg;
	struct sockaddr_nl src_addr;
	socklen_t addr_len = sizeof(src_addr);
	ssize_t ret;

	memset(&msg, 0, sizeof(msg));

	ret = recvfrom(sockfd, &msg, sizeof(msg), 0,
		       (struct sockaddr *)&src_addr, &addr_len);
	if (ret < 0) {
		perror("recvfrom");
		return -1;
	}

	if (ret < (ssize_t)sizeof(struct nlmsghdr)) {
		fprintf(stderr, "received message too short\n");
		return -1;
	}

	if (msg.hdr.nlmsg_len > (uint32_t)ret) {
		fprintf(stderr, "nlmsg_len larger than received data\n");
		return -1;
	}

	int data_len = msg.hdr.nlmsg_len - NLMSG_HDRLEN;
	if (data_len <= 0) {
		fprintf(stderr, "no data in message\n");
		return -1;
	}

	int copy_len = data_len < (int)buf_size - 1 ? data_len :
						      (int)buf_size - 1;
	memcpy(buffer, msg.data, copy_len);
	buffer[copy_len] = '\0';

	return copy_len;
}

static int test_basic(int sockfd)
{
	char recv_buf[MAX_PAYLOAD];
	const char *test_msg = "hello kernel";

	printf("=== Basic Test ===\n");

	if (send_to_kernel(sockfd, test_msg) < 0)
		return -1;

	printf("waiting for kernel response...\n");

	if (receive_from_kernel(sockfd, recv_buf, sizeof(recv_buf)) < 0)
		return -1;

	printf("received from kernel: %s\n", recv_buf);

	if (strcmp(recv_buf, test_msg) == 0) {
		printf("✓ echo test passed!\n");
	} else {
		printf("✗ echo test failed\n");
		return -1;
	}

	return 0;
}

static int test_loop(int sockfd)
{
	char recv_buf[MAX_PAYLOAD];
	const int num_messages = 5;
	int success_count = 0;
	int i;

	printf("=== Loop Test (%d messages) ===\n", num_messages);

	for (i = 0; i < num_messages; i++) {
		char test_msg[64];
		snprintf(test_msg, sizeof(test_msg), "test message #%d", i + 1);

		printf("\n[Message %d/%d]\n", i + 1, num_messages);

		if (send_to_kernel(sockfd, test_msg) < 0)
			continue;

		if (receive_from_kernel(sockfd, recv_buf, sizeof(recv_buf)) < 0)
			continue;

		printf("received from kernel: %s\n", recv_buf);

		if (strcmp(recv_buf, test_msg) == 0) {
			printf("✓ message %d echoed correctly\n", i + 1);
			success_count++;
		} else {
			printf("✗ message %d mismatch\n", i + 1);
		}
	}

	printf("\n=== Summary ===\n");
	printf("passed: %d/%d\n", success_count, num_messages);

	return (success_count == num_messages) ? 0 : -1;
}

static void usage(const char *prog)
{
	printf("Usage: %s [action]\n", prog);
	printf("\nActions:\n");
	printf("  0  - Basic echo test (default)\n");
	printf("  1  - Loop test with multiple messages\n");
}

int main(int argc, char **argv)
{
	int sockfd;
	int action = 0;
	int ret;

	if (argc > 1) {
		if (strcmp(argv[1], "-h") == 0 ||
		    strcmp(argv[1], "--help") == 0) {
			usage(argv[0]);
			return 0;
		}
		action = atoi(argv[1]);
	}

	sockfd = create_netlink_socket();
	if (sockfd < 0)
		return EXIT_FAILURE;

	if (bind_netlink_socket(sockfd) < 0) {
		close(sockfd);
		return EXIT_FAILURE;
	}

	printf("netlink socket created (protocol=%d, port=%d)\n", USER_MSG,
	       USER_PORT);

	switch (action) {
	case 0:
		ret = test_basic(sockfd);
		break;
	case 1:
		ret = test_loop(sockfd);
		break;
	default:
		fprintf(stderr, "unknown action: %d\n", action);
		usage(argv[0]);
		ret = -1;
	}

	close(sockfd);

	return (ret == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
