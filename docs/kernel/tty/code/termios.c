#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

static struct termios old_termios;

/* 恢复终端设置 */
static void restore_terminal(void)
{
	tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

/* 信号处理，防止异常退出破坏终端 */
static void handle_signal(int sig)
{
	(void)sig;
	restore_terminal();
	_exit(1);
}

int main(void)
{
	struct termios new_termios;
	char ch;

	/* 保存当前终端属性 */
	if (tcgetattr(STDIN_FILENO, &old_termios) < 0) {
		perror("tcgetattr");
		return 1;
	}

	/* 注册退出恢复 */
	atexit(restore_terminal);
	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	new_termios = old_termios;

	/*
         * 修改 termios：
         *  - 关闭回显 ECHO
         *  - 关闭规范模式 ICANON
         */
	new_termios.c_lflag &= ~(ECHO | ICANON);

	/*
         * VMIN = 1 : 至少读取 1 个字符返回
         * VTIME = 0 : 不设置超时
         */
	new_termios.c_cc[VMIN] = 1;
	new_termios.c_cc[VTIME] = 0;

	if (tcsetattr(STDIN_FILENO, TCSANOW, &new_termios) < 0) {
		perror("tcsetattr");
		return 1;
	}

	printf("termios demo: press 'q' to quit\r\n");

	/* 读取字符 */
	while (read(STDIN_FILENO, &ch, 1) == 1) {
		if (ch == 'q') {
			printf("\r\nquit\r\n");
			break;
		}

		printf("\r\nkey = 0x%02x ('%c')\r\n", ch,
		       (ch >= 32 && ch <= 126) ? ch : '.');
	}

	return 0;
}
