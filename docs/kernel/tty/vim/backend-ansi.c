#include "backend.h"

#include <stdio.h>
#include <string.h>

static void sequence(const char *text)
{
	backend_emit(text, strlen(text));
}

static void start(void)
{
	sequence("\033[?1049h");
	if (backend_flush("TX enter") < 0)
		fatal("write terminal");
}

static void stop(void)
{
	sequence("\033[0m\033[0 q\033[?25h\033[?1049l");
	(void)backend_flush("TX restore");
}

static void resize_screen(int rows, int cols)
{
	(void)rows;
	(void)cols;
}

static void begin(void)
{
	sequence("\033[?25l\033[0m");
}

static void position(int row, int col)
{
	char text[48];
	snprintf(text, sizeof(text), "\033[%d;%dH", row, col);
	sequence(text);
}

static void erase_line(void)
{
	sequence("\033[K");
}

static void erase_screen(void)
{
	sequence("\033[2J");
}

static void style(enum draw_style value)
{
	sequence(value == STYLE_CYAN   ? "\033[36m" :
		 value == STYLE_STATUS ? "\033[7m" :
					 "\033[0m");
}

static void present(bool normal)
{
	sequence(normal ? "\033[2 q" : "\033[6 q");
	sequence("\033[?25h");
	if (backend_flush("TX frame") < 0)
		fatal("write frame");
}

const struct backend backend_ansi = {
	.name = "ansi",
	.start = start,
	.stop = stop,
	.resize = resize_screen,
	.begin = begin,
	.position = position,
	.erase_line = erase_line,
	.erase_screen = erase_screen,
	.style = style,
	.text = backend_emit,
	.present = present,
};
