#ifndef MINI_VIM_BACKEND_H
#define MINI_VIM_BACKEND_H

#include <stdbool.h>
#include <stddef.h>

enum draw_style { STYLE_NORMAL, STYLE_CYAN, STYLE_STATUS };

/* Coordinates are one-based; text is already sanitized and clipped. */
struct backend {
	const char *name;
	void (*start)(void);
	void (*stop)(void);
	void (*resize)(int rows, int cols);
	void (*begin)(void);
	void (*position)(int row, int col);
	void (*erase_line)(void);
	void (*erase_screen)(void);
	void (*style)(enum draw_style style);
	void (*text)(const char *data, size_t len);
	void (*present)(bool normal);
};

extern const struct backend backend_ansi, backend_terminfo, backend_curses;

/* Shared byte buffer and trace, used by the two direct-output backends. */
void backend_emit(const char *data, size_t len);
int backend_flush(const char *label);
void trace_event(const char *format, ...);
_Noreturn void fatal(const char *operation);

#endif
