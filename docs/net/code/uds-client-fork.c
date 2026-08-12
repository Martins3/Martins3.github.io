/*
 * 测试一个内容，如果 fork 之后，parent 和 child 都持有一个 uds ，
 * 两者都去通过同一个 fd 发送消息，在 server 这里可以观察到两者都可以发消息。
 * 而且两者都可以接受消息。
 */
#include "uds.h"

int main(int argc, char *argv[])
{
	int sfd = unixConnect(SV_SOCK_PATH, SOCK_STREAM);
	if (sfd == -1)
		errExit("unixConnect");

	switch (fork()) {
	case -1:
		perror("fork\n");
		break;
	case 0:
		printf("child begin\n");
		write_to_server(sfd);
		break;
	default:
		printf("parent begin\n");
		write_to_server(sfd);
		break;
	}

	exit(EXIT_SUCCESS); /* Closes our socket; server sees EOF */
}
