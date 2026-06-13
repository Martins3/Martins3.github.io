/*
 * Check that CMD operations on sockets are consistent.
 *
 * 测试来自于:
 * https://lore.kernel.org/r/747d0519a2255bd055ae76b691d38d2b4c311001.1745843119.git.asml.silence@gmail.com
 *
 * 才理解 IORING_OP_URING_CMD 从简单的来说 ，其实就是提供一个 ioctl 的
 * 操作而已，但是如果搞复杂一点
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <linux/sockios.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <liburing.h>
#include <arpa/inet.h>

enum t_test_result {
	T_EXIT_PASS = 0,
	T_EXIT_FAIL = 1,
	T_EXIT_SKIP = 77,
};


/*
 * Helper for binding socket to an ephemeral port.
 * The port number to be bound is returned in @addr->sin_port.
 */
int t_bind_ephemeral_port(int fd, struct sockaddr_in *addr)
{
	socklen_t addrlen;
	int ret;

	addr->sin_port = 0;
	if (bind(fd, (struct sockaddr *)addr, sizeof(*addr)))
		return -errno;

	addrlen = sizeof(*addr);
	ret = getsockname(fd, (struct sockaddr *)addr, &addrlen);
	assert(!ret);
	assert(addr->sin_port != 0);
	return 0;
}

#define USERDATA 0xff0000
#define MSG "foobarbaz"
struct testsocks {
	/* Receive socket */
	int receive;
	/* Send socket */
	int send;
	/* accepted socket */
	int accepted;
};
/* Create 3 sockets (send, receive, and accepted) given the type */
static struct testsocks create_sockets(int type)
{
	struct sockaddr_in addr;
	struct testsocks retval;
	int32_t val = 1;
	int protocol = 0;
	int ret;
	if (type == SOCK_STREAM)
		protocol = IPPROTO_TCP;
	retval.receive = socket(AF_INET, type | SOCK_CLOEXEC, protocol);
	assert(retval.receive > 0);
	retval.send = socket(AF_INET, type | SOCK_CLOEXEC, protocol);
	assert(retval.send > 0);
	ret = setsockopt(retval.receive, SOL_SOCKET, SO_REUSEPORT, &val,
			 sizeof(val));
	assert(ret != -1);
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	assert(!t_bind_ephemeral_port(retval.receive, &addr));
	if (type == SOCK_STREAM) {
		ret = listen(retval.receive, 128);
		assert(ret != -1);
	}
	ret = connect(retval.send, (struct sockaddr *)&addr, sizeof(addr));
	assert(ret != -1);
	if (type == SOCK_STREAM) {
		retval.accepted = accept(retval.receive, NULL, NULL);
		assert(retval.accepted != -1);
	} else {
		retval.accepted = retval.receive;
	}
	return retval;
}
static struct io_uring create_ring()
{
	int ring_flags = 0;
	struct io_uring ring;
	int ret;
	ret = io_uring_queue_init(32, &ring, ring_flags);
	assert(ret >= 0);
	return ring;
}
static int create_sqe_and_submit(struct io_uring *ring, int32_t fd, int op)
{
	// Create socket
	struct io_uring_sqe *sqe;
	int ret;
	assert(fd > 0);
	sqe = io_uring_get_sqe(ring);
	assert(sqe != NULL);
	io_uring_prep_nop(sqe); // zeroing the struct
	sqe->opcode = IORING_OP_URING_CMD;
	sqe->user_data = USERDATA;
	sqe->fd = fd;
	sqe->cmd_op = op;
	/* Submitting SQE */
	ret = io_uring_submit_and_wait(ring, 1);
	assert(ret != -1);
	return 0;
}
static int receive_cqe(struct io_uring *ring)
{
	struct io_uring_cqe *cqe;
	int ret;
	ret = io_uring_wait_cqe(ring, &cqe);
	assert(ret == 0);
	assert(cqe->user_data == USERDATA);
	io_uring_cqe_seen(ring, cqe);
	/* Return the result of the operation */
	return cqe->res;
}
static ssize_t send_data(struct testsocks *s, char *str)
{
	return write(s->send, str, strlen(str));
}

static int run_test(int type)
{
	struct testsocks sockstruct;
	struct io_uring ring;
	size_t written_bytes;
	size_t bytes_in, bytes_out;
	/* Create three sockets */
	sockstruct = create_sockets(type);
	/* Send data sing the sockstruct->send */
	written_bytes = send_data(&sockstruct, MSG);
	assert(written_bytes == strlen(MSG));
	/* Simply io_uring ring creation */
	ring = create_ring();
	create_sqe_and_submit(&ring, sockstruct.accepted,
			      SOCKET_URING_OP_SIOCINQ);
	bytes_in = receive_cqe(&ring);
	create_sqe_and_submit(&ring, sockstruct.send, SOCKET_URING_OP_SIOCOUTQ);
	bytes_out = receive_cqe(&ring);
	if (bytes_in == -ENOTSUP || bytes_out == -ENOTSUP) {
		fprintf(stderr, "Skipping tests. -ENOTSUP returned\n");
		return T_EXIT_SKIP;
	}
	/* Assert the number of written bytes are either in the socket buffer
	 * or on the receive side */
	assert(bytes_in + bytes_out == written_bytes);
	io_uring_queue_exit(&ring);
	return T_EXIT_PASS;
}
static int run_test_raw()
{
	int ioctl_value, ioctl_value1;
	int uring_value, uring_value1;
	struct io_uring ring;
	int sock, error;
	sock = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
	if (sock <= 0) {
		perror("Not able to create a raw socket");
		return T_EXIT_SKIP;
	}
	/* Simple SIOCOUTQ using ioctl */
	error = ioctl(sock, SIOCOUTQ, &ioctl_value);
	if (error < 0) {
		fprintf(stderr, "Failed to run ioctl(SIOCOUTQ): %x\n", error);
		return T_EXIT_FAIL;
	}
	error = ioctl(sock, SIOCINQ, &ioctl_value1);
	if (error < 0) {
		fprintf(stderr, "Failed to run ioctl(SIOCINQ): %x\n", error);
		return T_EXIT_FAIL;
	}
	/* Get the same operation using uring cmd */
	ring = create_ring();
	create_sqe_and_submit(&ring, sock, SOCKET_URING_OP_SIOCOUTQ);
	uring_value = receive_cqe(&ring);
	create_sqe_and_submit(&ring, sock, SOCKET_URING_OP_SIOCINQ);
	uring_value1 = receive_cqe(&ring);
	/* Compare that both values (ioctl and uring CMD) should be similar */
	assert(uring_value == ioctl_value);
	assert(uring_value1 == ioctl_value1);
	return T_EXIT_PASS;
}
int main(int argc, char *argv[])
{
	int ret;
	if (argc > 1)
		return 0;
	srand(getpid());
	ret = run_test(SOCK_STREAM);
	if (ret)
		return ret;
	ret = run_test(SOCK_DGRAM);
	if (ret)
		return ret;
	ret = run_test_raw();
	return T_EXIT_PASS;
}
