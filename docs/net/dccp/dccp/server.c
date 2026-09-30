#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>

#include "dccp.h"

#define PORT 1337
#define SERVICE_CODE 42

int error_exit(const char *str)
{
	perror(str);
	exit(errno);
}

int main(int argc, char **argv)
{
	int listen_sock = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP);
	if (listen_sock < 0)
		error_exit("socket");

	struct sockaddr_in servaddr = {
		.sin_family = AF_INET,
		.sin_addr.s_addr = htonl(INADDR_ANY),
		.sin_port = htons(PORT),
	};

	if (setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)))
		error_exit("setsockopt(SO_REUSEADDR)");

	if (bind(listen_sock, (struct sockaddr *)&servaddr, sizeof(servaddr)))
		error_exit("bind");

	// DCCP mandates the use of a 'Service Code' in addition the port
	if (setsockopt(listen_sock, SOL_DCCP, DCCP_SOCKOPT_SERVICE, &(int) {
		       htonl(SERVICE_CODE)}, sizeof(int)))
		error_exit("setsockopt(DCCP_SOCKOPT_SERVICE)");

	if (listen(listen_sock, 1))
		error_exit("listen");

	for (;;) {

		printf("Waiting for connection...\n");

		struct sockaddr_in client_addr;
		socklen_t addr_len = sizeof(client_addr);

		int conn_sock = accept(listen_sock, (struct sockaddr *)&client_addr, &addr_len);
		if (conn_sock < 0) {
			perror("accept");
			continue;
		}

		printf("Connection received from %s:%d\n",
		       inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

		for (;;) {
			char buffer[1024];
			// Each recv() will read only one individual message.
			// Datagrams, not a stream!
			int ret = recv(conn_sock, buffer, sizeof(buffer), 0);
			if (ret > 0)
				printf("Received: %s\n", buffer);
			else
				break;

		}

		close(conn_sock);
	}
}
