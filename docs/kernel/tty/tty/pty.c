/*
 *
 * 1. screen 不太完美，但是在 screen 的哪一侧写任何东西最后都可以在 master 真的可以收到
 * 但是依旧足够我来理解这个东西了
 *
 * 2. pty-slave 也可以使用，可以在 master 中看到
 * bash: cannot set terminal process group (-1): Inappropriate ioctl for device
 * bash: no job control in this shell
 * 但是我没有办法 enter
 *
 * 3. 使用 socat socat -d -d STDIO /dev/pts/19
 * 完全回显正常的
 *
 * 4. 使用 setsid 的做法，这个效果就更加神奇了
 * setsid sh -c 'exec bash </dev/pts/19 >/dev/pts/19 2>&1'
 *
 * */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>

int main()
{
	int master_fd, slave_fd;
	char slave_name[256];

	// Open a pseudo-terminal pair
	master_fd = posix_openpt(O_RDWR | O_NOCTTY);
	if (master_fd == -1) {
		perror("posix_openpt");
		exit(EXIT_FAILURE);
	}

	// Grant access to the slave
	if (grantpt(master_fd) == -1) {
		perror("grantpt");
		close(master_fd);
		exit(EXIT_FAILURE);
	}

	// Unlock the slave
	if (unlockpt(master_fd) == -1) {
		perror("unlockpt");
		close(master_fd);
		exit(EXIT_FAILURE);
	}

	// Get the name of the slave
	if (ptsname_r(master_fd, slave_name, sizeof(slave_name)) != 0) {
		perror("ptsname_r");
		close(master_fd);
		exit(EXIT_FAILURE);
	}

	printf("PTY created:\n");
	printf("  Master FD: %d\n", master_fd);
	printf("  Slave: %s\n", slave_name);
	printf(" screen %s 115200\n", slave_name);

	// Open the slave side
	slave_fd = open(slave_name, O_RDWR | O_NOCTTY);
	if (slave_fd == -1) {
		perror("open slave");
		close(master_fd);
		exit(EXIT_FAILURE);
	}

	printf("Slave opened successfully\n");

	// Set terminal attributes for raw mode on both ends
	struct termios tio;
	if (tcgetattr(master_fd, &tio) == 0) {
		cfmakeraw(&tio);
		tcsetattr(master_fd, TCSANOW, &tio);
	}

	if (tcgetattr(slave_fd, &tio) == 0) {
		cfmakeraw(&tio);
		tcsetattr(slave_fd, TCSANOW, &tio);
	}

	// Simple echo test: read from master, write to slave and vice versa
	char buffer[256];
	fd_set readfds;

	printf("Starting simple PTY echo test. Type something:\n");

	while (1) {
		FD_ZERO(&readfds);
		FD_SET(STDIN_FILENO, &readfds); // Standard input
		FD_SET(master_fd, &readfds); // PTY master

		int maxfd = (STDIN_FILENO > master_fd) ? STDIN_FILENO :
							 master_fd;

		if (select(maxfd + 1, &readfds, NULL, NULL, NULL) > 0) {
			// Data from stdin to PTY master
			if (FD_ISSET(STDIN_FILENO, &readfds)) {
				ssize_t n = read(STDIN_FILENO, buffer,
						 sizeof(buffer));
				if (n > 0) {
					write(master_fd, buffer, n);
				}
			}

			// Data from PTY master to stdout
			if (FD_ISSET(master_fd, &readfds)) {
				ssize_t n =
					read(master_fd, buffer, sizeof(buffer));
				if (n > 0) {
					write(STDOUT_FILENO, buffer, n);
				}
			}
		}
	}

	// Close file descriptors (normally won't reach here due to infinite loop)
	close(master_fd);
	close(slave_fd);

	return 0;
}
