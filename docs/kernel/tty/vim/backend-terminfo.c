#include "backend.h"

#include <curses.h>
#include <term.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static char *cup, *el, *clear_cap, *sgr0, *rev, *setaf;
static char *smcup, *rmcup, *civis, *cnorm, *shape, *reset_shape;
static bool ready;
static int screen_rows = 24;

static char *capability(const char *name)
{
	char *value = tigetstr(name);
	return value == (char *)-1 ? NULL : value;
}

static int output_char(int value)
{
	char byte = (char)value;
	backend_emit(&byte, 1);
	return value;
}

static void put_cap(const char *value, int affected)
{
	if (value)
		(void)tputs(value, affected, output_char);
}

static void start(void)
{
	int error;
	if (setupterm(NULL, STDOUT_FILENO, &error) == ERR) {
		fprintf(stderr,
			"mini-vim: terminfo cannot load TERM=%s (status %d)\n",
			getenv("TERM") ? getenv("TERM") : "(unset)", error);
		errno = ENOTSUP;
		fatal("setupterm");
	}
	cup = capability("cup");
	el = capability("el");
	clear_cap = capability("clear");
	sgr0 = capability("sgr0");
	if (!cup || !el || !clear_cap || !sgr0) {
		(void)del_curterm(cur_term);
		errno = ENOTSUP;
		fatal("terminfo requires cup, el, clear and sgr0");
	}
	rev = capability("rev");
	setaf = tigetnum("colors") >= 8 ? capability("setaf") : NULL;
	smcup = capability("smcup");
	rmcup = capability("rmcup");
	civis = capability("civis");
	cnorm = capability("cnorm");
	shape = capability("Ss");
	reset_shape = capability("Se");
	ready = true;
	put_cap(smcup, 1);
	if (backend_flush("TX enter") < 0)
		fatal("write terminal");
	trace_event("TERMINFO: cup/el/clear/sgr0 loaded; color=%d shape=%d",
		    setaf != NULL, shape != NULL);
}

static void stop(void)
{
	if (!ready)
		return;
	put_cap(sgr0, 1);
	put_cap(reset_shape, 1);
	put_cap(cnorm, 1);
	put_cap(rmcup, 1);
	(void)backend_flush("TX restore");
	(void)del_curterm(cur_term);
	ready = false;
}

static void resize_screen(int rows, int cols)
{
	screen_rows = rows;
	(void)cols;
}

static void begin(void)
{
	put_cap(civis, 1);
	put_cap(sgr0, 1);
}

static void position(int row, int col)
{
	put_cap(tparm(cup, (long)row - 1, (long)col - 1), 1);
}

static void erase_line(void)
{
	put_cap(el, 1);
}

static void erase_screen(void)
{
	put_cap(clear_cap, screen_rows);
}

static void style(enum draw_style value)
{
	put_cap(sgr0, 1);
	if (value == STYLE_CYAN && setaf)
		put_cap(tparm(setaf, 6L), 1);
	else if (value == STYLE_STATUS)
		put_cap(rev, 1);
}

static void present(bool normal)
{
	put_cap(cnorm, 1);
	if (shape)
		put_cap(tparm(shape, normal ? 2L : 6L), 1);
	if (backend_flush("TX frame") < 0)
		fatal("write frame");
}

const struct backend backend_terminfo = {
	.name = "terminfo",
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
