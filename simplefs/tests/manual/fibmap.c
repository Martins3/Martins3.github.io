#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/fs.h>
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
	const char *path = argc > 1 ? argv[1] : "/mnt/simplefs/file";
	int block = 0;
	int fd;

	fd = open(path, O_RDONLY);
	if (fd < 0) {
		perror("open");
		return 1;
	}

	if (ioctl(fd, FIBMAP, &block) < 0) {
		perror("ioctl FIBMAP");
		close(fd);
		return 1;
	}

	printf("%s: logical block 0 -> physical block %d\n", path, block);
	close(fd);
	return 0;
}
