/*
 * server 使用
 * nc -u -l 11346
 * 来测试
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/net_tstamp.h>
#include <netdb.h>

#define UDP_PORTNO 11346

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
	perror(msg);
	exit(0);
}

int main(int argc, char **argv)
{
	int sockfd, portno = UDP_PORTNO, n;
	unsigned int serverlen;
	struct sockaddr_in serveraddr;
	struct hostent *server;
	char *hostname = "10.0.0.108";
	char buf[BUFSIZE];

	/* check command line arguments */
	if (argc != 2)
		error("./udp_client [hostname]\n");
	hostname = argv[1];

	/* socket: create the socket */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	int flags = SOF_TIMESTAMPING_TX_HARDWARE;
	int ret = setsockopt(sockfd, SOL_SOCKET, SO_TIMESTAMPING, &flags,
			     sizeof(flags));
	if (ret == -1)
		error("setsockopt");

	server = gethostbyname(hostname);
	if (server == NULL)
		error("ERROR, no such host\n");

	/* build the server's Internet address */
	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr,
	      server->h_length);
	serveraddr.sin_port = htons(portno);

	bzero(buf, BUFSIZE);
	sprintf(buf, ".\n");
	/* send the message to the server */
	serverlen = sizeof(serveraddr);
	n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr *)&serveraddr,
		   serverlen);
	if (n < 0)
		error("ERROR in sendto");
	return 0;
}
