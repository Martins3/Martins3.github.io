/*
 * https://man7.org/tlpi/code/online/dist/sockets/us_xfr_v2_sv.c.html
 */
#include "uds.h"

int unixBind(const char *path, int type)
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

int main(int argc, char *argv[])
{
	// 先删掉之前的，不然 bind 会报错，原因未知
	unlink(SV_SOCK_PATH);
	// TODO SOCK_STREAM 和 SOCK_DGRAM 在 uds 的区别是什么?
	/* int sfd = unixBind(SV_SOCK_PATH, SOCK_DGRAM); */
	int sfd = unixBind(SV_SOCK_PATH, SOCK_STREAM);
	if (sfd == -1)
		errExit("unixBind");

	if (listen(sfd, 5) == -1)
		errExit("listen");

	for (;;) { /* Handle client connections iteratively */
		int cfd = accept(sfd, NULL, NULL);
		if (cfd == -1)
			errExit("accept");

		/* Transfer data from connected socket to stdout until EOF */

		ssize_t numRead;
		ssize_t numWrite;
		char buf[BUF_SIZE];

		while ((numRead = read(cfd, buf, BUF_SIZE)) > 0) {
			printf("%s\n", buf);
			numWrite = write(cfd, buf, BUF_SIZE);
			if (numWrite != BUF_SIZE)
				errExit("write failed\n");
		}

		if (numRead == -1)
			errExit("read");

		if (close(cfd) == -1)
			errExit("close");
	}
}
