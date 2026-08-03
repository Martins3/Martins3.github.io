#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

// clang 可以将这个函数直接优化到没有，
int flag = 1;
void badwait_flag()
{
	while (!flag) {
	}
}

int main(int argc, char *argv[])
{
	// 即便是在这里设置了 flag = 1
	flag = 1;
	badwait_flag();
	return EXIT_SUCCESS;
}
