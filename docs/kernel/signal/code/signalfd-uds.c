/*
 * 测试通过 uds 传递 signalfd ，但是
 * 似乎并不支持 signalfd 的传递，
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/signalfd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#define SOCKET_PATH "/tmp/signal_socket"

// Send file descriptor over UDS
int send_fd(int sockfd, int fd_to_send) {
  struct msghdr msg = {0};
  struct iovec iov[1];
  char buf[1] = {0}; // Dummy data
  iov[0].iov_base = buf;
  iov[0].iov_len = sizeof(buf);

  union {
    struct cmsghdr cm;
    char control[CMSG_SPACE(sizeof(int))];
  } control_un;
  struct cmsghdr *cmptr;

  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  msg.msg_control = control_un.control;
  msg.msg_controllen = sizeof(control_un.control);

  cmptr = CMSG_FIRSTHDR(&msg);
  cmptr->cmsg_len = CMSG_LEN(sizeof(int));
  cmptr->cmsg_level = SOL_SOCKET;
  cmptr->cmsg_type = SCM_RIGHTS;
  *((int *)CMSG_DATA(cmptr)) = fd_to_send;

  if (sendmsg(sockfd, &msg, 0) == -1) {
    perror("sendmsg");
    return -1;
  }
  return 0;
}

// Receive file descriptor over UDS
int recv_fd(int sockfd) {
  struct msghdr msg = {0};
  struct iovec iov[1];
  char buf[1];
  iov[0].iov_base = buf;
  iov[0].iov_len = sizeof(buf);

  union {
    struct cmsghdr cm;
    char control[CMSG_SPACE(sizeof(int))];
  } control_un;
  struct cmsghdr *cmptr;

  msg.msg_iov = iov;
  msg.msg_iovlen = 1;
  msg.msg_control = control_un.control;
  msg.msg_controllen = sizeof(control_un.control);

  if (recvmsg(sockfd, &msg, 0) == -1) {
    perror("recvmsg");
    return -1;
  }

  if ((cmptr = CMSG_FIRSTHDR(&msg)) != NULL &&
      cmptr->cmsg_len == CMSG_LEN(sizeof(int)) &&
      cmptr->cmsg_level == SOL_SOCKET && cmptr->cmsg_type == SCM_RIGHTS) {
    return *((int *)CMSG_DATA(cmptr));
  }

  fprintf(stderr, "No file descriptor received\n");
  return -1;
}

void run_server() {
  int sockfd, clientfd;
  struct sockaddr_un addr;

  // Create UDS socket
  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // Set up address structure
  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

  // Remove existing socket file
  unlink(SOCKET_PATH);

  // Bind socket
  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) ==
      -1) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  // Listen for connections
  if (listen(sockfd, 5) == -1) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  printf("Server running, waiting for client connection...\n");

  // Accept client connection
  clientfd = accept(sockfd, NULL, NULL);
  if (clientfd == -1) {
    perror("accept");
    exit(EXIT_FAILURE);
  }

  // Receive signalfd from client
  int client_sfd = recv_fd(clientfd);
  if (client_sfd == -1) {
    fprintf(stderr, "Failed to receive signalfd\n");
    exit(EXIT_FAILURE);
  }

  printf("Server received client's signalfd, monitoring for signals...\n");

  while (1) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(client_sfd, &readfds);

    // Wait for signals on client's signalfd
    int ret = select(client_sfd + 1, &readfds, NULL, NULL, NULL);
    if (ret == -1) {
      if (errno == EINTR)
        continue;
      perror("select");
      exit(EXIT_FAILURE);
    }

    printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
    if (FD_ISSET(client_sfd, &readfds)) {
      struct signalfd_siginfo si;
      ssize_t bytes = read(client_sfd, &si, sizeof(si));
      if (bytes != sizeof(si)) {
        fprintf(stderr, "Read error\n");
        exit(EXIT_FAILURE);
      }

      if (si.ssi_signo == SIGINT) {
        printf("Server: Client received SIGINT (Ctrl+C)\n");
      } else if (si.ssi_signo == SIGTERM) {
        printf("Server: Client received SIGTERM\n");
      } else {
        printf("Server: Client received unexpected signal %d\n", si.ssi_signo);
      }

      if (si.ssi_signo == SIGTERM) {
        printf("Server: Exiting due to SIGTERM...\n");
        break;
      }
    }
  }

  close(client_sfd);
  close(clientfd);
  close(sockfd);
  unlink(SOCKET_PATH);
}

void run_client() {
  int sfd, sockfd;
  sigset_t mask;
  struct sockaddr_un addr;

  // Set up signal mask
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT);

  if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
    perror("sigprocmask");
    exit(EXIT_FAILURE);
  }

  // Create signalfd
  sfd = signalfd(-1, &mask, 0);
  if (sfd == -1) {
    perror("signalfd");
    exit(EXIT_FAILURE);
  }

  // Create UDS socket
  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (sockfd == -1) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  // Set up address structure
  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path) - 1);

  // Connect to server
  if (connect(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_un)) ==
      -1) {
    perror("connect");
    exit(EXIT_FAILURE);
  }

  // Send signalfd to server
  if (send_fd(sockfd, sfd) == -1) {
    fprintf(stderr, "Failed to send signalfd\n");
    exit(EXIT_FAILURE);
  }

  printf("Client sent signalfd to server, waiting for signals...\n");

  // Keep client running until terminated
  while (1) {
    printf("[martins3:%s:%d] \n", __FUNCTION__, __LINE__);
    sleep(1);
  }

  close(sfd);
  close(sockfd);
}

int main(int argc, char *argv[]) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s [-s | -c]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  if (strcmp(argv[1], "-s") == 0) {
    run_server();
  } else if (strcmp(argv[1], "-c") == 0) {
    run_client();
  } else {
    fprintf(stderr, "Invalid argument. Use -s for server or -c for client\n");
    exit(EXIT_FAILURE);
  }

  return 0;
}
