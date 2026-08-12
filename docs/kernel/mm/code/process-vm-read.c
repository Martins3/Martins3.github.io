// 测试 https://lwn.net/Articles/789153/ 提到的内容
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <sys/wait.h>

#define TARGET_STRING "Hello from target process!"
#define REPLACE_STRING "Hello from source process!"

int main()
{
	pid_t pid;
	int status;
	char buffer[256] = { 0 };

	pid = fork();
	if (pid == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid == 0) {
		printf("Target process (PID: %d)\n", getpid());
		printf("Original string: %s (at address %p)\n", TARGET_STRING, TARGET_STRING);

		printf("Target process waiting...\n");
		sleep(30);

		printf("Target process exiting...\n");
		exit(EXIT_SUCCESS);
	} else { 
		sleep(1); 

		printf("Source process (PID: %d)\n", getpid());

		struct iovec local[1];
		struct iovec remote[1];

		local[0].iov_base = buffer;
		local[0].iov_len = sizeof(buffer);

		remote[0].iov_base = (void *)TARGET_STRING;
		remote[0].iov_len = strlen(TARGET_STRING) + 1;

		// 测试 process_vm_readv
		ssize_t nread = process_vm_readv(pid, local, 1, remote, 1, 0);
		if (nread == -1) {
			perror("process_vm_readv");
			exit(EXIT_FAILURE);
		}

		printf("Read %zd bytes from target process: %s\n", nread,
		       buffer);

		// 进行测试 process_vm_writev
		const char *new_data = REPLACE_STRING;
		size_t data_len = strlen(new_data) + 1;

		local[0].iov_base = (void *)new_data;
		local[0].iov_len = data_len;

		remote[0].iov_base = (void *)TARGET_STRING;
		remote[0].iov_len = data_len;

		// FIXME 是 ds 产生的，但是这里跑不通
		// 好好看看这个 syscall 为什么要加进去，感觉好危险啊，一个 process 随便的写任何 process 的地址
		ssize_t nwritten =
			process_vm_writev(pid, local, 1, remote, 1, 0);
		if (nwritten == -1) {
			perror("process_vm_writev");
			exit(EXIT_FAILURE);
		}

		printf("Wrote %zd bytes to target process\n", nwritten);

		memset(buffer, 0, sizeof(buffer));
		nread = process_vm_readv(pid, local, 1, remote, 1, 0);
		if (nread == -1) {
			perror("process_vm_readv");
			exit(EXIT_FAILURE);
		}

		printf("Read %zd bytes after write: %s\n", nread, buffer);

		waitpid(pid, &status, 0);
	}

	return 0;
}
