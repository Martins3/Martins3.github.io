// tcgetattr_demo.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <string.h>

int main(void)
{
	struct termios t;

	// 获取标准输入（通常是当前终端）的 termios 属性
	if (tcgetattr(STDIN_FILENO, &t) == -1) {
		fprintf(stderr, "tcgetattr failed: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	printf("=== Current terminal (stdin) settings ===\n");

	// -------- c_iflag (input modes) --------
	printf("Input flags (c_iflag):\n");
	printf("  IGNBRK: %s\n", (t.c_iflag & IGNBRK) ? "ON" : "off");
	printf("  BRKINT: %s\n", (t.c_iflag & BRKINT) ? "ON" : "off");
	printf("  IGNPAR: %s\n", (t.c_iflag & IGNPAR) ? "ON" : "off");
	printf("  PARMRK: %s\n", (t.c_iflag & PARMRK) ? "ON" : "off");
	printf("  INPCK:  %s\n", (t.c_iflag & INPCK) ? "ON" : "off");
	printf("  ISTRIP: %s\n", (t.c_iflag & ISTRIP) ? "ON" : "off");
	printf("  INLCR:  %s\n", (t.c_iflag & INLCR) ? "ON" : "off");
	printf("  IGNCR:  %s\n", (t.c_iflag & IGNCR) ? "ON" : "off");
	printf("  ICRNL:  %s\n", (t.c_iflag & ICRNL) ? "ON" : "off");
	printf("  IXON:   %s\n", (t.c_iflag & IXON) ? "ON" : "off");

	// -------- c_oflag (output modes) --------
	printf("\nOutput flags (c_oflag):\n");
	printf("  OPOST: %s\n", (t.c_oflag & OPOST) ? "ON" : "off");

	// -------- c_cflag (control modes) --------
	printf("\nControl flags (c_cflag):\n");
	// 波特率（需用 cfgetispeed / cfgetospeed）
	speed_t ispeed = cfgetispeed(&t);
	speed_t ospeed = cfgetospeed(&t);
	printf("  Input speed:  %d\n", (int)ispeed);
	printf("  Output speed: %d\n", (int)ospeed);

	// 数据位
	if ((t.c_cflag & CSIZE) == CS5)
		printf("  Data bits: 5\n");
	else if ((t.c_cflag & CSIZE) == CS6)
		printf("  Data bits: 6\n");
	else if ((t.c_cflag & CSIZE) == CS7)
		printf("  Data bits: 7\n");
	else if ((t.c_cflag & CSIZE) == CS8)
		printf("  Data bits: 8\n");

	// 停止位和校验
	printf("  CSTOPB (2 stop bits): %s\n",
	       (t.c_cflag & CSTOPB) ? "ON" : "off");
	printf("  PARENB (parity enable): %s\n",
	       (t.c_cflag & PARENB) ? "ON" : "off");
	printf("  PARODD (odd parity): %s\n",
	       (t.c_cflag & PARODD) ? "ON" : "off");

	// -------- c_lflag (local modes) --------
	printf("\nLocal flags (c_lflag):\n");
	printf("  ICANON: %s\n", (t.c_lflag & ICANON) ? "ON" : "off");
	printf("  ECHO:   %s\n", (t.c_lflag & ECHO) ? "ON" : "off");
	printf("  ECHOE:  %s\n", (t.c_lflag & ECHOE) ? "ON" : "off");
	printf("  ISIG:   %s\n", (t.c_lflag & ISIG) ? "ON" : "off");

	// -------- c_cc (special characters) --------
	printf("\nSpecial characters (decimal values):\n");
	printf("  VMIN:  %d\n", t.c_cc[VMIN]);
	printf("  VTIME: %d\n", t.c_cc[VTIME]);
	// 可选：打印回车、换行、中断等
	printf("  VERASE: 0x%02x ('%c')\n", t.c_cc[VERASE], t.c_cc[VERASE]);
	printf("  VINTR:  0x%02x ('%c')\n", t.c_cc[VINTR], t.c_cc[VINTR]);

	return 0;
}
