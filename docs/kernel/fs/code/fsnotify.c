// generatey by deepseek
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <limits.h>

// 事件缓冲区大小
#define EVENT_SIZE (sizeof(struct inotify_event))
#define BUF_LEN (1024 * (EVENT_SIZE + NAME_MAX + 1))

int main(int argc, char *argv[])
{
	if (argc < 2) {
		fprintf(stderr, "Usage: %s <directory>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	// 初始化 inotify
	int inotify_fd = inotify_init();
	if (inotify_fd == -1) {
		perror("inotify_init");
		exit(EXIT_FAILURE);
	}

	// 添加监控目录
	const char *dir_path = argv[1];
	int watch_fd = inotify_add_watch(inotify_fd, dir_path,
					 IN_CREATE | IN_DELETE | IN_MODIFY);
	if (watch_fd == -1) {
		perror("inotify_add_watch");
		close(inotify_fd);
		exit(EXIT_FAILURE);
	}

	printf("Monitoring directory: %s\n", dir_path);

	// 事件缓冲区
	char buffer[BUF_LEN];

	while (1) {
		// 读取事件
		ssize_t num_read = read(inotify_fd, buffer, BUF_LEN);
		if (num_read == -1) {
			perror("read");
			break;
		}

		// 处理事件
		for (char *ptr = buffer; ptr < buffer + num_read;) {
			struct inotify_event *event =
				(struct inotify_event *)ptr;

			// 打印事件信息
			if (event->len) {
				if (event->mask & IN_CREATE) {
					printf("File created: %s\n",
					       event->name);
				}
				if (event->mask & IN_DELETE) {
					printf("File deleted: %s\n",
					       event->name);
				}
				if (event->mask & IN_MODIFY) {
					printf("File modified: %s\n",
					       event->name);
				}
			}

			ptr += EVENT_SIZE + event->len;
		}
	}

	// 清理
	inotify_rm_watch(inotify_fd, watch_fd);
	close(inotify_fd);

	return 0;
}
