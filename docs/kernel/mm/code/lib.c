#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include "rmap.h"

#define SV_SOCK_PATH "us_xfr_v2_uds_lib"
#define BUF_SIZE 100

/*
 * https://man7.org/tlpi/code/online/dist/sockets/us_xfr_v2.h.html
 */
static int unixBuildAddress(const char *path, struct sockaddr_un *addr)
{
	if (addr == NULL || path == NULL) {
		errno = EINVAL;
		return -1;
	}

	memset(addr, 0, sizeof(struct sockaddr_un));
	addr->sun_family = AF_UNIX;
	if (strlen(path) < sizeof(addr->sun_path)) {
		strncpy(addr->sun_path, path, sizeof(addr->sun_path) - 1);
		return 0;
	} else {
		errno = ENAMETOOLONG;
		return -1;
	}
}

static inline void errExit(const char *format, ...)
{
	fprintf(stderr, "%s\n", format);
	exit(1);
}

static int unixConnect(const char *path, int type)
{
	struct sockaddr_un addr;

	if (unixBuildAddress(path, &addr) == -1)
		return -1;

	int sd = socket(AF_UNIX, type, 0);
	if (sd == -1)
		return -1;

	/* getsockopt(socketHandle, IPPROTO_TCP, TCP_NODELAY, */
	/* 	   (char *)&iSocketOption, &iSocketOptionLen); */

	if (connect(sd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) ==
	    -1) {
		int savedErrno = errno;
		close(sd); /* Might change 'errno' */
		errno = savedErrno;
		return -1;
	}

	return sd;
}
int child_main()
{
	int sfd = unixConnect(SV_SOCK_PATH, SOCK_STREAM);
	if (sfd == -1)
		errExit("unixConnect");

	/* Copy stdin to socket */

	ssize_t numRead;
	char buf[BUF_SIZE];

	printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
	while ((numRead = read(sfd, buf, BUF_SIZE)) > 0) {
		child_handler(buf);
	}

	if (numRead == -1)
		errExit("read");

	printf("Child process (PID: %d) exit\n", getpid());
	exit(EXIT_SUCCESS); /* Closes our socket; server sees EOF */
}

static int unixBind(const char *path, int type)
{
	struct sockaddr_un addr;

	if (unixBuildAddress(path, &addr) == -1)
		return -1;

	int sd = socket(AF_UNIX, type, 0);
	if (sd == -1)
		return -1;

	if (bind(sd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) ==
	    -1) {
		int savedErrno = errno;
		close(sd); /* Might change 'errno' */
		errno = savedErrno;
		return -1;
	}

	return sd;
}

struct Arg {
	int sfd;
} arg;

/*
 * 第一个 fd 是 1
 */
int child_fd[1024];
int fd_max_idx = 0;

static void add_fd(int fd)
{
	fd_max_idx++;
	child_fd[fd_max_idx] = fd;
	printf("thread number : %d\n", fd_max_idx);
}

static int get_fd(int idx)
{
	return child_fd[idx];
}

static int check_fd(int idx)
{
	if (idx <= 0)
		goto err;

	if (idx > fd_max_idx)
		goto err;
	return 0;
err:
	printf("not valid\n");
	return -1;
}

static void *thread_func(void *no)
{
	for (;;) {
		int cfd = accept(arg.sfd, NULL, NULL);
		if (cfd == -1)
			errExit("accept");
		add_fd(cfd);
	}
	return NULL;
}

int parent_main()
{
	int sfd = unixBind(SV_SOCK_PATH, SOCK_STREAM);

	if (sfd == -1)
		errExit("unixBind");

	if (listen(sfd, 1024) == -1)
		errExit("listen");

	arg.sfd = sfd;

	pthread_t thread;
	int ret = pthread_create(&thread, NULL, thread_func, NULL);
	if (ret != 0) {
		perror("pthread_create\n");
		return 1;
	}

	printf("Parent process (PID: %d) \n", getpid());
	char cmd[128];
	int id;
	while (scanf("%d %s", &id, cmd) > 0) {
		if (id == 0) {
			printf("bye\n");
			exit(0);
		}
		if (check_fd(id))
			continue;
		int len = write(get_fd(id), cmd, strlen(cmd) + 1);
		if (len != strlen(cmd) + 1)
			errExit("partial/failed write");
	}
	return 0;
}
