#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MIN_RAND_NUM 0
#define MAX_RAND_NUM 100

int tty_fd;

// copied from https://github.com/shuveb/linux-tidbits/blob/master/02_process_group_signals/add_nums.c
// TODO why /dev/tty
void open_tty() {

  // TODO 
  tty_fd = open("/dev/tty", O_WRONLY);
  if (tty_fd < 0) {
    fprintf(stderr, "Error opening TTY.\n");
    exit(1);
  }else{
    printf("tty_fd is %d\n", tty_fd);
  }
}

void print_to_tty(char *s) {
  int len = strlen(s);
  write(tty_fd, s, len);
}

void sigint_handler(int signo) {
  print_to_tty("get_nums got SIGINT. Will exit...\n");
  exit(0);
}

void print_process_group_info() {
  char str[128];
  memset(str, 0, sizeof(str));
  snprintf(str, sizeof(str), "gen_nums: PID: %d, PGID: %d\n", getpid(),
           getpgrp());
  print_to_tty(str);
}

int main() {
  print_process_group_info();
  srand(time(NULL));

  open_tty();
  signal(SIGINT, sigint_handler);

  while (1) {
    int rand_num1 = (rand() % (MAX_RAND_NUM - MIN_RAND_NUM)) + MIN_RAND_NUM;
    int rand_num2 = (rand() % (MAX_RAND_NUM - MIN_RAND_NUM)) + MIN_RAND_NUM;
    printf("%d %d\n", rand_num1, rand_num2);
    fflush(stdout);
    sleep(1);
  }

  return 0;
}
