#include "backend.h"

#include <curses.h>
#include <errno.h>
#include <stdio.h>

static SCREEN *screen;
static bool color;

static void check(int result, const char *operation)
{
	if (result == ERR) {
		errno = EIO;
		fatal(operation);
	}
}

static void start(void)
{
	/* Size comes from the shared TIOCGWINSZ path, not LINES/COLUMNS. */
	use_env(FALSE);
	screen = newterm(NULL, stdout, stdin);
	if (!screen) {
		errno = ENOTSUP;
		fatal("newterm (check TERM and terminfo database)");
	}
	check(raw(), "curses raw");
	check(noecho(), "curses noecho");
	check(nonl(), "curses nonl");
	/* Input remains read()/poll(); do not defer drawing for pending input. */
	check(typeahead(-1), "curses typeahead");
	if (has_colors() && start_color() != ERR && COLORS >= 8) {
		short background = use_default_colors() == OK ? -1 :
								COLOR_BLACK;
		color = init_pair(1, COLOR_CYAN, background) == OK;
	}
	trace_event("CURSES: newterm; ncursesw virtual screen; color=%d",
		    color);
}

static void stop(void)
{
	if (!screen)
		return;
	(void)curs_set(1);
	(void)endwin();
	delscreen(screen);
	screen = NULL;
}

static void resize_screen(int rows, int cols)
{
	if (getmaxy(stdscr) != rows || getmaxx(stdscr) != cols)
		check(resizeterm(rows, cols), "curses resizeterm");
}

static void style(enum draw_style value)
{
	chtype attr = value == STYLE_STATUS	   ? A_REVERSE :
		      value == STYLE_CYAN && color ? COLOR_PAIR(1) :
						     A_NORMAL;
	wbkgdset(stdscr, ' ' | attr);
	check(wattrset(stdscr, attr), "curses wattrset");
}

static void begin(void)
{
	style(STYLE_NORMAL);
	/* Erase the virtual window only; doupdate still compares physical cells. */
	check(werase(stdscr), "curses werase");
}

static void position(int row, int col)
{
	check(wmove(stdscr, row - 1, col - 1), "curses wmove");
}

static void erase_line(void)
{
	check(wclrtoeol(stdscr), "curses wclrtoeol");
}

static void erase_screen(void)
{
	check(werase(stdscr), "curses werase");
}

static void text(const char *data, size_t len)
{
	if (len)
		check(waddnstr(stdscr, data, (int)len), "curses waddnstr");
}

static void present(bool normal)
{
	/* Portable curses offers cursor visibility, not the xterm bar shape. */
	(void)curs_set(normal ? 2 : 1);
	check(wnoutrefresh(stdscr), "curses wnoutrefresh");
	check(doupdate(), "curses doupdate");
	if (ferror(stdout)) {
		errno = EIO;
		fatal("curses output");
	}
	trace_event(
		"CURSES frame: wnoutrefresh + doupdate (not a TX byte dump)");
}

const struct backend backend_curses = {
	.name = "curses",
	.start = start,
	.stop = stop,
	.resize = resize_screen,
	.begin = begin,
	.position = position,
	.erase_line = erase_line,
	.erase_screen = erase_screen,
	.style = style,
	.text = text,
	.present = present,
};
