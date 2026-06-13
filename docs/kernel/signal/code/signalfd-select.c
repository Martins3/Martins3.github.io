/*
 * signalfd 的简单测试
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/signalfd.h>
#include <unistd.h>

int main() {
  // 必须把传统的信号默认屏蔽掉才可以，不然传统的信号会更早的接受到
  sigset_t mask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGINT); // Ctrl+C

  if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
    perror("sigprocmask");
    exit(EXIT_FAILURE);
  }

  int sfd = signalfd(-1, &mask, 0);
  if (sfd == -1) {
    perror("signalfd");
    exit(EXIT_FAILURE);
  }

  printf("Waiting for signals (SIGINT or SIGTERM)...\n");

  while (1) {
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(sfd, &readfds);

    // 使用 select 等待信号
    int ret = select(sfd + 1, &readfds, NULL, NULL, NULL);
    if (ret == -1) {
      if (errno == EINTR)
        continue;
      perror("select");
      exit(EXIT_FAILURE);
    }

    if (FD_ISSET(sfd, &readfds)) {
      struct signalfd_siginfo si;
      ssize_t bytes = read(sfd, &si, sizeof(si));
      if (bytes != sizeof(si)) {
        fprintf(stderr, "Read error\n");
        exit(EXIT_FAILURE);
      }

      if (si.ssi_signo == SIGINT) {
        printf("Received SIGINT (Ctrl+C)\n");
        continue;
      } else {
        printf("Received unexpected signal %d\n", si.ssi_signo);
      }

      printf("Exiting...\n");
      break;
    }
  }

  close(sfd);
  return 0;
}
