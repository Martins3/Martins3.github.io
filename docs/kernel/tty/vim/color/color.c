#include <stdio.h>
#include <stdlib.h>

#define CSI "\x1b["
#define RESET CSI "0m"

static void print_basic_colors(void)
{
	static const char *const names[] = {
		"black", "red", "green", "yellow",
		"blue", "magenta", "cyan", "white",
	};

	puts("Basic SGR foreground colors:");
	for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); i++)
		printf(CSI "%zum%-8s" RESET "%s", i + 30, names[i],
		       i == 7 ? "\n" : " ");
}

int main(void)
{
	print_basic_colors();

	printf(CSI "0;33mSGR 33: named yellow" RESET "\n");
	printf(CSI "38;5;208mSGR 38;5;208: indexed color 208" RESET "\n");
	printf(CSI "38;2;255;100;40mSGR 38;2: truecolor RGB(255, 100, 40)" RESET "\n");

	return ferror(stdout) ? EXIT_FAILURE : EXIT_SUCCESS;
}
