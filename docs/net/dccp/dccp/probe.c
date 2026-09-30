#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "dccp.h"

int main()
{
	int sock_fd = socket(AF_INET, SOCK_DCCP, IPPROTO_DCCP);

	// Check the congestion control schemes available
	socklen_t res_len = 6;
	uint8_t ccids[6];
	if (getsockopt(sock_fd, SOL_DCCP, DCCP_SOCKOPT_AVAILABLE_CCIDS, ccids, &res_len)) {
		perror("getsockopt(DCCP_SOCKOPT_AVAILABLE_CCIDS)");
		return -1;
	}

	printf("%d CCIDs available:", res_len);
	for (int i = 0; i < res_len; i++)
		printf(" %d", ccids[i]);

	return res_len;
}
