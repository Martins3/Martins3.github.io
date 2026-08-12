/*
 * https://www.cs.cmu.edu/afs/cs/academic/class/15213-f99/www/class26/udpserver.c
 * */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <linux/net_tstamp.h>
#include <arpa/inet.h>
#include "udp.h"

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg)
{
	perror(msg);
	exit(1);
}

int main(int argc, char **argv)
{
	int sockfd; /* socket */
	int portno = UDP_PORTNO; /* port to listen on */
	unsigned int clientlen; /* byte size of client's address */
	struct sockaddr_in addr; /* server's addr */
	struct sockaddr_in clientaddr; /* client addr */
	struct hostent *hostp; /* client host info */
	char buf[BUFSIZE]; /* message buf */
	char *hostaddrp; /* dotted decimal host addr string */
	int optval; /* flag value for setsockopt */
	int n; /* message byte size */

	/*
	 * socket: create the parent socket
         */
	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0)
		error("ERROR opening socket");

	int flags = SOF_TIMESTAMPING_RX_HARDWARE;
	int ret = setsockopt(sockfd, SOL_SOCKET, SO_TIMESTAMPING, &flags,
			     sizeof(flags));
	if (ret == -1) {
		perror("setsockopt");
		close(sockfd);
		exit(1);
	}

	/* setsockopt: Handy debugging trick that lets
	 * us rerun the server immediately after we kill it;
   	 * otherwise we have to wait about 20 secs.
   	 * Eliminates "ERROR on binding: Address already in use" error.
   	 */
	optval = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval,
		   sizeof(int));

	/*
         * build the server's Internet address
         */
	bzero((char *)&addr, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons((unsigned short)portno);

	/*
         * bind: associate the parent socket with a port
         */
	if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0)
		error("ERROR on binding");

	/*
	 * main loop: wait for a datagram, then echo it
         */
	clientlen = sizeof(clientaddr);
	while (1) {
		/*
		 * recvfrom: receive a UDP datagram from a client
		 */
		bzero(buf, BUFSIZE);
		n = recvfrom(sockfd, buf, BUFSIZE, 0,
			     (struct sockaddr *)&clientaddr, &clientlen);
		if (n < 0)
			error("ERROR in recvfrom");

		/*
		 * gethostbyaddr: determine who sent the datagram
		 */

		/*
		 * TODO 这一套如果使用了如果用了特殊的网络环境，那么
		 * 似乎都是有问题的? 例如 server 是 xueshi-ubuntu 的时候
		 *
		 * 背后的原理是什么?
		 */
		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
				      sizeof(clientaddr.sin_addr.s_addr),
				      AF_INET);
		if (hostp == NULL)
			error("ERROR on gethostbyaddr");
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
			error("ERROR on inet_ntoa\n");
		printf("server received datagram from %s (%s)\n", hostp->h_name,
		       hostaddrp);
		printf("server received %ld/%d bytes: %s\n", strlen(buf), n,
		       buf);


		sprintf(buf, "-\n");
		n = sendto(sockfd, buf, strlen(buf), 0,
			   (struct sockaddr *)&clientaddr, clientlen);
		if (n < 0)
			error("ERROR in sendto");
	}
}
