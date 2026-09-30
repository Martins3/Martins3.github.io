// tcsetattr_demo.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

int main(void)
{
	struct termios orig_termios, raw_termios;
	char buf[10];
	ssize_t n;

	// 1. 获取当前终端属性（保存用于恢复）
	if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) {
		perror("tcgetattr");
		exit(EXIT_FAILURE);
	}

	// 2. 复制一份用于修改
	raw_termios = orig_termios;

	// 3. 设置 raw 模式（禁用 canonical、回显、信号等）
	raw_termios.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR |
				 IGNCR | ICRNL | IXON);
	raw_termios.c_oflag &= ~OPOST;
	raw_termios.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	raw_termios.c_cflag &= ~(CSIZE | PARENB);
	raw_termios.c_cflag |= CS8; // 8-bit characters

	// 设置非阻塞最小读取：立即返回（即使 1 字节）
	raw_termios.c_cc[VMIN] = 1;
	raw_termios.c_cc[VTIME] = 0;

	// 4. 应用新设置（TCSAFLUSH：丢弃输入/输出队列并应用）
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_termios) == -1) {
		perror("tcsetattr");
		exit(EXIT_FAILURE);
	}

	printf("进入 raw 模式！按任意 3 个键（输入不会回显）...\n");
	fflush(stdout);

	// 5. 读取 3 个原始字节（例如按 'a'、方向键、Ctrl+C 等）
	n = read(STDIN_FILENO, buf, sizeof(buf) - 1);
	if (n > 0) {
		buf[n] = '\0';
		printf("\n你按下的原始字节（十六进制）：");
		for (ssize_t i = 0; i < n; i++) {
			printf("%02x ", (unsigned char)buf[i]);
		}
		printf("\n");
	}

	// 6. 复原始终端设置（重要！否则终端会“卡住”）
	if (tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios) == -1) {
		perror("tcsetattr restore");
		exit(EXIT_FAILURE);
	}

	printf("已恢复原始终端设置，程序退出。\n");
	return 0;
}
