#ifndef UDS_JHFLBSDF_H
#define UDS_JHFLBSDF_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#define SV_SOCK_PATH "us_xfr_v2"

#define BUF_SIZE 100

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

static inline int unixConnect(const char *path, int type)
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

static inline void write_to_server(int sfd)
{
	char buf[BUF_SIZE];
	sprintf(buf, "%d send to server\n", getpid());
	while (1) {
		if (write(sfd, buf, sizeof(buf)) != sizeof(buf))
			errExit("partial/failed write");

		if (read(sfd, buf, BUF_SIZE) != sizeof(buf))
			errExit("partial/failed read");

		printf("%d : %s\n", getpid(), buf);
		sleep(1);
	}
}

#endif /* end of include guard: FILE_H */
