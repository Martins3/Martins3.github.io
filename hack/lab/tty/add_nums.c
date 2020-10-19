#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

#define MIN_RAND_NUM 0
#define MAX_RAND_NUM 100

void sigint_handler(int signo) {
    printf("add_nums got SIGINT. Will exit...\n");
    exit(0);
}

void print_process_group_info() {
    printf("add_nums: PID: %d, PGID: %d\n", getpid(), getpgrp());
}

int main() {
    int num1, num2;
    char str1[1024];
    char str2[1024];

    print_process_group_info();

    signal(SIGINT, sigint_handler);

    while(1) {
        fscanf(stdin, "%d %d\n", &num1, &num2);
        printf("sum is %d\n", num1 + num2);
    }

    return 0;
}
