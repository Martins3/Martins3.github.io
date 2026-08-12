/*
 * https://stackoverflow.com/questions/28003921/sending-file-descriptor-by-linux-socket
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

static void err_syserr(const char *msg)
{
	fprintf(stderr, "%s\n", msg);
	exit(1);
}

static void wyslij(int socket, int fd) // send fd by socket
{
	struct msghdr msg = { 0 };
	char buf[CMSG_SPACE(sizeof(fd))];
	memset(buf, '\0', sizeof(buf));

	/* On Mac OS X, the struct iovec is needed, even if it points to minimal data */
	struct iovec io = { .iov_base = "", .iov_len = 1 };

	msg.msg_iov = &io;
	msg.msg_iovlen = 1;
	msg.msg_control = buf;
	msg.msg_controllen = sizeof(buf);

	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);
	cmsg->cmsg_level = SOL_SOCKET;
	cmsg->cmsg_type = SCM_RIGHTS;
	cmsg->cmsg_len = CMSG_LEN(sizeof(fd));

	memmove(CMSG_DATA(cmsg), &fd, sizeof(fd));

	msg.msg_controllen = CMSG_SPACE(sizeof(fd));

	if (sendmsg(socket, &msg, 0) < 0)
		err_syserr("Failed to send message\n");
}

static int odbierz(int socket) // receive fd from socket
{
	struct msghdr msg = { 0 };

	/* On Mac OS X, the struct iovec is needed, even if it points to minimal data */
	char m_buffer[1];
	struct iovec io = { .iov_base = m_buffer, .iov_len = sizeof(m_buffer) };
	msg.msg_iov = &io;
	msg.msg_iovlen = 1;

	char c_buffer[256];
	msg.msg_control = c_buffer;
	msg.msg_controllen = sizeof(c_buffer);

	if (recvmsg(socket, &msg, 0) < 0)
		err_syserr("Failed to receive message\n");

	struct cmsghdr *cmsg = CMSG_FIRSTHDR(&msg);

	printf("About to extract fd\n");
	int fd;
	memmove(&fd, CMSG_DATA(cmsg), sizeof(fd));
	printf("Extracted fd %d\n", fd);

	return fd;
}

int main(int argc, char **argv)
{
	const char *filename = "./z7.c";
	if (argc > 1)
		filename = argv[1];
	int sv[2];
	// TODO 这里的 socketpair 如何理解?
	if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) != 0)
		err_syserr("Failed to create Unix-domain socket pair\n");

	int pid = fork();
	if (pid > 0) // in parent
	{
		printf("Parent at work\n");
		close(sv[1]);
		int sock = sv[0];

		int fd = open(filename, O_RDONLY);
		if (fd < 0)
			err_syserr("Failed to open file for reading\n");

		/* Read some data to demonstrate that file offset is passed */
		char buffer[32];
		int nbytes = read(fd, buffer, sizeof(buffer));
		if (nbytes > 0)
			printf("Parent read: [[%.*s]]\n", nbytes, buffer);

		wyslij(sock, fd);

		close(fd);
		nanosleep(&(struct timespec){ .tv_sec = 1,
					      .tv_nsec = 500000000 },
			  0);
		printf("Parent exits\n");
	} else // in child
	{
		printf("Child at play\n");
		close(sv[0]);
		int sock = sv[1];

		nanosleep(&(struct timespec){ .tv_sec = 0,
					      .tv_nsec = 500000000 },
			  0);

		int fd = odbierz(sock);
		printf("Read %d!\n", fd);
		char buffer[256];
		ssize_t nbytes;
		while ((nbytes = read(fd, buffer, sizeof(buffer))) > 0)
			fprintf(stdout, buffer, nbytes);
		printf("Done!\n");
		close(fd);
	}
	return 0;
}
