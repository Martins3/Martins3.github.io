#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main(int argc, char **argv)
{
	char *hostname = "10.0.0.2";
	/* check command line arguments */
	if (argc == 2)
		hostname = argv[1];
	else if (argc == 1)
		printf("use default host %s\n", hostname);
	else {
		printf("./tcp_client [hostname]\n");
		exit(1);
	}

	int port = 5566;

	int sock;
	struct sockaddr_in addr;
	char buffer[1024];
	int ret = 0;

	sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// sock = socket(AF_INET, SOCK_STREAM, IPPROTO_MPTCP);
	if (sock < 0) {
		perror("[-]Socket error");
		exit(1);
	}
	printf("[+]TCP server socket created.\n");

	memset(&addr, '\0', sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	addr.sin_addr.s_addr = inet_addr(hostname);

	connect(sock, (struct sockaddr *)&addr, sizeof(addr));
	printf("Connected to the server.\n");

	while (1) {
		bzero(buffer, 1024);
		strcpy(buffer, "HELLO, THIS IS CLIENT.");
		ret = send(sock, buffer, strlen(buffer), 0);
		if (ret < 0) {
			printf("send failed\n");
			exit(1);
		}
		printf("Client: %s\n", buffer);

		bzero(buffer, 1024);
		recv(sock, buffer, sizeof(buffer), 0);
		printf("Server: %s\n", buffer);
		sleep(3);
	}

	close(sock);
	printf("Disconnected from the server.\n");

	return 0;
}
