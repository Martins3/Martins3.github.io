#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>

int main()
{
	int port = 5566;

	int server_sock, client_sock;
	struct sockaddr_in server_addr, client_addr;
	socklen_t addr_size;
	char buffer[1024];
	int n;

	server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	// server_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_MPTCP);
	if (server_sock < 0) {
		perror("[-]Socket error");
		exit(1);
	}
	printf("[+]TCP server socket created.\n");

	memset(&server_addr, '\0', sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = port;
	/* TODO 如果这里 bind 127.0.0.1 ，那么跨机器的 tcp 将无法联通，符合预期，但是
	 * 可以看看 bind 的具体如何处理的
	 *
	 * 似乎如果不去设置，那么默认的是任何地址。
	 */
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	n = bind(server_sock, (struct sockaddr *)&server_addr,
		 sizeof(server_addr));
	if (n < 0) {
		perror("[-]Bind error");
		exit(1);
	}
	printf("[+]Bind to the port number: %d\n", port);

	listen(server_sock, 5);
	printf("Listening...\n");

	addr_size = sizeof(client_addr);
	client_sock = accept(server_sock, (struct sockaddr *)&client_addr,
			     &addr_size);
	printf("[+]Client connected.\n");

	while (1) {
		bzero(buffer, 1024);
		recv(client_sock, buffer, sizeof(buffer), 0);
		printf("Client: %s\n", buffer);

		bzero(buffer, 1024);
		strcpy(buffer, "HI, THIS IS SERVER. HAVE A NICE DAY!!!");
		printf("Server: %s\n", buffer);
		send(client_sock, buffer, strlen(buffer), 0);
	}
	close(client_sock);
	printf("[+]Client disconnected.\n\n");

	return 0;
}
