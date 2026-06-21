#define _XOPEN_SOURCE 600
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>
#include <limits.h>

static void die(const char *msg)
{
	perror(msg);
	exit(1);
}

int main(int argc, char *argv[])
{
	if (argc != 2)
		die("./pty-slave.out number");
	int pty = atoi(argv[1]);
	char path[PATH_MAX];
	sprintf(path, "/dev/pts/%d", pty);

	setsid(); /* 新会话，摆脱控制终端 */
	int slave = open(path, O_RDWR);
	if (slave < 0)
		die("open slave");

	/* stdin/stdout/stderr -> PTY slave */
	dup2(slave, STDIN_FILENO);
	dup2(slave, STDOUT_FILENO);
	dup2(slave, STDERR_FILENO);

	close(slave);

	execlp("bash", "bash", NULL);
	die("execlp");
}
