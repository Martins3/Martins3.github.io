/*
 * https://man7.org/tlpi/code/online/dist/sockets/us_xfr_v2.h.html
 */
#include "uds.h"

int main(int argc, char *argv[])
{
	int sfd = unixConnect(SV_SOCK_PATH, SOCK_STREAM);
	if (sfd == -1)
		errExit("unixConnect");

	write_to_server(sfd);

	exit(EXIT_SUCCESS); /* Closes our socket; server sees EOF */
}
