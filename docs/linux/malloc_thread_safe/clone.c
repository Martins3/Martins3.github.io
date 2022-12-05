#define _GNU_SOURCE
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <sched.h>
#include <sys/mman.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdlib.h>

static int childFunc(void *arg)
{
	printf("Child:  PID=%ld PPID=%ld\n", (long)getpid(), (long)getppid());
  while(true){
    void * x = malloc(0x10);
    free(x);
  }
	return 0;
}

int guest_clone()
{
	const int STACK_SIZE = 1 << 14; /* Stack size for cloned child */
	char *stack; /* Start of stack buffer area */
	char *stackTop; /* End of stack buffer area */
	int flags = 0; /* Flags for cloning child */
	int status;
	pid_t pid = -1;
	int args;

	printf("Parent: PID=%ld PPID=%ld\n", (long)getpid(), (long)getppid());

	flags |= CLONE_VM;
	// flags |= CLONE_FILES;
	// flags |= CLONE_FS;
	// flags |= CLONE_SIGHAND;

	/* Allocate stack for child */

	stack = mmap(NULL, STACK_SIZE, PROT_READ | PROT_WRITE,
		     MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
	if (stack == MAP_FAILED)
		return 111;

	stackTop = stack + STACK_SIZE; /* Assume stack grows downward */

	printf("child new stack %p in guest_clone\n", stackTop);

	if (clone(childFunc, stackTop, flags | SIGCHLD, &args) == -1)
		return 112;

	/* Parent falls through to here. Wait for child; __WCLONE option is
       required for child notifying with signal other than SIGCHLD. */

  while(true){
    void * x = malloc(0x10);
    free(x);
  }

	pid = waitpid(-1, &status, __WALL);
	if (pid == -1)
		return 0;

	printf("    Child PID=%ld\n", (long)pid);

	return 0;
}

int main(){
  guest_clone();
  return 0;
}
