#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "dccp.h"

int error_exit(const char *str)
{
	perror(str);
	exit(errno);
}

int main(int argc, char *argv[])
{
	if (argc < 5) {
		printf("Usage: ./client <server address> <port> <service code> "
		       "<message 1> [message 2] ... \n");
		exit(-1);
	}
	struct sockaddr_in server_addr = {
		.sin_family = AF_INET,
		.sin_port = htons(atoi(argv[2])),
	};

	if (!inet_pton(AF_INET, argv[1], &server_addr.sin_addr.s_addr)) {
		printf("Invalid address %s\n", argv[1]);
		exit(-1);
	}

	int socket_fd = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP);
	if (socket_fd < 0)
		error_exit("socket");

	if (setsockopt(socket_fd, SOL_DCCP, DCCP_SOCKOPT_SERVICE,
		       &(int){ htonl(atoi(argv[3])) }, sizeof(int)))
		error_exit("setsockopt(DCCP_SOCKOPT_SERVICE)");

	if (connect(socket_fd, (struct sockaddr *)&server_addr,
		    sizeof(server_addr)))
		error_exit("connect");

	// Get the maximum packet size
	uint32_t mps;
	socklen_t res_len = sizeof(mps);
	if (getsockopt(socket_fd, SOL_DCCP, DCCP_SOCKOPT_GET_CUR_MPS, &mps,
		       &res_len))
		error_exit("getsockopt(DCCP_SOCKOPT_GET_CUR_MPS)");
	printf("Maximum Packet Size: %d\n", mps);

	for (int i = 4; i < argc; i++) {
		if (send(socket_fd, argv[i], strlen(argv[i]) + 1, 0) < 0)
			error_exit("send");
	}

	// Wait for a while to allow all the messages to be transmitted
	usleep(5 * 1000);

	close(socket_fd);
	return 0;
}
