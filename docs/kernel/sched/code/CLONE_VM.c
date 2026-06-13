#define _GNU_SOURCE
/*
 * code is copied from
 * https://embeddedguruji.blogspot.com/2018/12/clonevm-example.html
 *
 * # clone CLONE_VM flag 的作用
 * <!-- 2aa0323c-5e23-43fc-b541-e58794345fc9 -->
 *
 * If CLONE_VM is set, the calling process and the child process run in the  same  memory space.
 * If CLONE_THREAD is set, the child is placed in the same thread group as the calling process.
 *
 * Since  Linux  2.5.35, the flags mask must also include CLONE_SIGHAND if CLONE_THREAD is specified
 * (and  note  that,  since  Linux  2.6.0, CLONE_SIGHAND also requires CLONE_VM to be included).
 *
 * 也就是说，CLONE_THREAD 一定要求有 CLONE_VM ，但是 CLONE_VM 可以不要求 CLONE_THREAD，
 * 这个导致了一个诡异的结果，两个 process 是可以共享地址空间的
 *
 * ./CLONE_VM.out vm # 共享
 *
 * ```txt
 *   pstree -p 3491523
 * zsh(3491523)───CLONE_VM.out(3517866)───CLONE_VM.out(3517867)
 *
 * 🧀  ps -o tid,pid,tpgid,sid -p 3517866
 *     TID     PID   TPGID     SID
 * 3517866 3517866 3517866 3491523
 *
 * 🧀  ps -o tid,pid,tpgid,sid -p 3517867
 *     TID     PID   TPGID     SID
 * 3517867 3517867 3517866 3491523
 *
 * ./CLONE_VM.out # 不共享
 *
 *   pstree -p 3491523
 * zsh(3491523)───CLONE_VM.out(3519560)───CLONE_VM.out(3519561)
 *
 * 🧀  ps -o tid,pid,tpgid,sid -p 3519560
 *     TID     PID   TPGID     SID
 * 3519560 3519560 3519560 3491523
 *
 * 🧀  ps -o tid,pid,tpgid,sid -p 3519561
 *     TID     PID   TPGID     SID
 * 3519561 3519561 3519560 3491523
 *
 * 可见，两个 process 是可以共享地址空间的
 */
#include <stdio.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define STACK_SIZE 65536

int global_value = 0;
char *heap;
static int child_func(void *arg)
{
	char *buf = (char *)arg;
	printf("Child sees buf = %s\n", buf);
	printf("Child sees global value = %d\n", global_value);
	printf("Child see heap = %s\n", heap);
	strcpy(buf, "hello from child");
	global_value = 10;
	strcpy(heap, "bye");
	sleep(1000);
	return 0;
}

int main(int argc, char *argv[])
{
	//Allocate stack for child task
	char *stack = malloc(STACK_SIZE);
	unsigned long flags = 0;
	char buf[256];
	int status;

	if (!stack) {
		perror("Failed to allocate memory\n");
		exit(1);
	}
	heap = malloc(1024);
	if (!heap) {
		perror("Failed to allocate memory\n");
		exit(2);
	}
	if (argc == 2 && !strcmp(argv[1], "vm"))
		flags |= CLONE_VM;
	strcpy(buf, "Hello from Parent");
	strcpy(heap, "Hey");
	global_value = 5;
	if (clone(child_func, stack + STACK_SIZE, flags | SIGCHLD, buf) == -1) {
		perror("clone");
		exit(1);
	}
	if (wait(&status) == -1) {
		perror("Wait");
		exit(1);
	}
	printf("Child exited with status:%d\t", status);
	printf("buf:%s\t global_value=%d\n", buf, global_value);
	printf("Parent heap:%s\n", heap);
	return 0;
}
