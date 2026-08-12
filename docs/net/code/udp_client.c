// TODO 记录下这个 : https://stackoverflow.com/questions/21099041/why-do-we-cast-sockaddr-in-to-sockaddr-when-calling-bind

/*
 * https://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/udpclient.c
 */
/*
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
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
#include "udp.h"

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
	if (argc == 2)
		hostname = argv[1];
	else if (argc == 1)
		printf("use default host %s\n", hostname);
	else {
		error("./udp_client [hostname]\n");
		exit(1);
	}

	/* socket: create the socket */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	int flags = SOF_TIMESTAMPING_TX_HARDWARE;
	int ret = setsockopt(sockfd, SOL_SOCKET, SO_TIMESTAMPING, &flags,
			     sizeof(flags));
	if (ret == -1) {
		perror("setsockopt");
		close(sockfd);
		exit(1);
	}

	// TODO 如果想要带硬件的 timestamp ，virtio-net 可以吗?
	// 似乎不可以

	/* gethostbyname: get the server's DNS entry */
	// TODO 这个是在调用什么东西，似乎就是，
	//
	server = gethostbyname(hostname);
	if (server == NULL) {
		fprintf(stderr, "ERROR, no such host as %s\n", hostname);
		exit(0);
	}

	/* build the server's Internet address */
	bzero((char *)&serveraddr, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	bcopy((char *)server->h_addr, (char *)&serveraddr.sin_addr.s_addr,
	      server->h_length);
	serveraddr.sin_port = htons(portno);

	for (;;) {
		bzero(buf, BUFSIZE);
		sprintf(buf, ".\n");
		/* send the message to the server */
		serverlen = sizeof(serveraddr);
		n = sendto(sockfd, buf, strlen(buf), 0,
			   (struct sockaddr *)&serveraddr, serverlen);
		if (n < 0)
			error("ERROR in sendto");

		/* print the server's reply */
		n = recvfrom(sockfd, buf, strlen(buf), 0,
			     (struct sockaddr *)&serveraddr, &serverlen);
		if (n < 0)
			error("ERROR in recvfrom");
		printf("%s", buf);
		sleep(1);
	}
	return 0;
}
