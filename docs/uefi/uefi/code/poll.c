/**
 * poll 相关的测试
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/poll.h>
#include <time.h>

int main(int argc, char *argv[]) {
  int numPipes, ready, j;
  struct pollfd *pollFd;
  int pfds[1]; /* File descriptors for all pipes */

  /* Allocate the arrays that we use. The arrays are sized according
     to the number of pipes specified on command line */

  numPipes = 1;

  pollFd = calloc(numPipes, sizeof(struct pollfd));
  if (pollFd == NULL) {
    exit(EXIT_FAILURE);
  }

  /* Create the number of pipes specified on command line */

  for (j = 0; j < numPipes; j++)
    pfds[j] = 0;

  /* Build the file descriptor list to be supplied to poll(). This list
     is set to contain the file descriptors for the read ends of all of
     the pipes. */

  for (j = 0; j < numPipes; j++) {
    pollFd[j].fd = pfds[j];
    pollFd[j].events = POLLIN;
  }

  printf("huxueshi:%s before \n", __FUNCTION__);
  ready = poll(pollFd, numPipes, -1);
  printf("huxueshi:%s after \n", __FUNCTION__);
  if (ready == -1) {
    exit(EXIT_FAILURE);
  }

  printf("poll() returned: %d\n", ready);

  /* Check which pipes have data available for reading */

  for (j = 0; j < numPipes; j++)
    if (pollFd[j].revents & POLLIN) {
      printf("Readable: %3d\n", pollFd[j].fd);
      char x[100];
      scanf("%s", x);
      printf("[%s]", x);
    }

  exit(EXIT_SUCCESS);
}
