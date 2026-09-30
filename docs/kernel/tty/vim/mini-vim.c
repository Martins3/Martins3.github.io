#define _XOPEN_SOURCE 700

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <poll.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <wchar.h>

#include "backend.h"

/* Editing and input are shared; only terminal drawing is replaceable. */
#define FILE_LIMIT (8U * 1024U * 1024U)

static const struct backend *backend = &backend_ansi;

enum {
	KEY_NONE = 256,
	KEY_UP,
	KEY_DOWN,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_HOME,
	KEY_END,
	KEY_DELETE,
	KEY_PASTE_BEGIN,
	KEY_PASTE_END
};
enum mode { NORMAL, INSERT, COMMAND };

struct buffer {
	char *data;
	size_t len, cap;
};

static struct {
	struct buffer text, frame;
	size_t cursor, top, left;
	int rows, cols;
	unsigned int resizes;
	enum mode mode;
	bool dirty, quit, terminal_active, paste;
	int prefix;
	char command[PATH_MAX + 16], message[PATH_MAX + 128];
	size_t command_len;
	char *path;
	struct termios saved_termios;
	FILE *trace;
} editor;

static volatile sig_atomic_t resized;
static volatile sig_atomic_t stopped;

void trace_event(const char *format, ...)
{
	if (!editor.trace)
		return;
	va_list args;
	va_start(args, format);
	vfprintf(editor.trace, format, args);
	va_end(args);
	fputc('\n', editor.trace);
	fflush(editor.trace);
}

/* 日志是可读转义表示；真正发给终端的 ESC 是一个 0x1b 字节。 */
static void trace_bytes(const char *direction, const char *data, size_t len)
{
	if (!editor.trace)
		return;
	fprintf(editor.trace, "%s %zu bytes: ", direction, len);
	for (size_t i = 0; i < len; i++) {
		unsigned char c = (unsigned char)data[i];
		if (c == 27)
			fputs("<ESC>", editor.trace);
		else if (c == '\r')
			fputs("\\r", editor.trace);
		else if (c == '\n')
			fputs("\\n", editor.trace);
		else if (c == '\t')
			fputs("\\t", editor.trace);
		else if (c >= 32 && c < 127)
			fputc(c, editor.trace);
		else
			fprintf(editor.trace, "\\x%02x", c);
	}
	fputc('\n', editor.trace);
	fflush(editor.trace);
}

static int write_all(int fd, const char *data, size_t len)
{
	while (len) {
		ssize_t n = write(fd, data, len);
		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			return -1;
		data += n;
		len -= (size_t)n;
	}
	return 0;
}

static void terminal_restore(void)
{
	if (!editor.terminal_active)
		return;
	editor.terminal_active = false;
	/* Restore input even if a backend's output cleanup cannot allocate memory. */
	while (tcsetattr(STDIN_FILENO, TCSANOW, &editor.saved_termios) < 0) {
		if (errno != EINTR)
			break;
	}
	editor.frame.len = 0;
	const char sequence[] = "\033[?2004l";
	trace_bytes("TX paste off", sequence, sizeof(sequence) - 1);
	(void)write_all(STDOUT_FILENO, sequence, sizeof(sequence) - 1);
	backend->stop();
}

_Noreturn void fatal(const char *operation)
{
	int saved = errno;
	terminal_restore();
	fprintf(stderr, "mini-vim: %s: %s\n", operation, strerror(saved));
	exit(EXIT_FAILURE);
}

static void reserve(struct buffer *buffer, size_t need)
{
	if (need <= buffer->cap)
		return;
	size_t cap = buffer->cap ? buffer->cap : 1024;
	while (cap < need)
		cap *= 2;
	char *data = realloc(buffer->data, cap);
	if (!data)
		fatal("realloc");
	buffer->data = data;
	buffer->cap = cap;
}

static void append(struct buffer *buffer, const char *data, size_t len)
{
	reserve(buffer, buffer->len + len + 1);
	memcpy(buffer->data + buffer->len, data, len);
	buffer->len += len;
	buffer->data[buffer->len] = '\0';
}

static void emit(const char *text)
{
	backend->text(text, strlen(text));
}

void backend_emit(const char *data, size_t len)
{
	append(&editor.frame, data, len);
}

int backend_flush(const char *label)
{
	trace_bytes(label, editor.frame.data, editor.frame.len);
	int result = write_all(STDOUT_FILENO, editor.frame.data, editor.frame.len);
	editor.frame.len = 0;
	return result;
}

static void message(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	vsnprintf(editor.message, sizeof(editor.message), format, args);
	va_end(args);
}

/* 信号处理器只设置标志；ioctl、分配内存和绘制都在主循环中进行。 */
static void signal_handler(int signo)
{
	if (signo == SIGWINCH)
		resized = 1;
	else
		stopped = signo;
}

static void terminal_start(void)
{
	if (!isatty(STDIN_FILENO) || !isatty(STDOUT_FILENO)) {
		fprintf(stderr,
			"mini-vim: stdin and stdout must be a terminal\n");
		exit(EXIT_FAILURE);
	}
	if (tcgetattr(STDIN_FILENO, &editor.saved_termios) < 0)
		fatal("tcgetattr");
	struct sigaction action = { .sa_handler = signal_handler };
	sigemptyset(&action.sa_mask);
	const int signals[] = { SIGWINCH, SIGINT, SIGTERM, SIGHUP, SIGQUIT };
	for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); i++) {
		if (sigaction(signals[i], &action, NULL) < 0)
			fatal("sigaction");
	}
	action.sa_handler = SIG_IGN;
	if (sigaction(SIGPIPE, &action, NULL) < 0)
		fatal("sigaction SIGPIPE");

	struct termios raw = editor.saved_termios;
	raw.c_iflag &= ~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR |
			 ICRNL | IXON);
	raw.c_oflag &= ~OPOST;
	raw.c_lflag &= ~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
	raw.c_cflag &= ~(CSIZE | PARENB);
	raw.c_cflag |= CS8;
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;
	if (atexit(terminal_restore) != 0) {
		errno = ENOMEM;
		fatal("atexit");
	}
	editor.terminal_active = true;
	trace_event("BACKEND: %s", backend->name);
	backend->start();
	if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0)
		fatal("tcsetattr");
	trace_event("RAW: ICANON=0 ECHO=0 ISIG=0 OPOST=0 VMIN=1 VTIME=0");
	/* Shared xterm extension: the input parser is identical in all backends. */
	const char sequence[] = "\033[?2004h";
	trace_bytes("TX paste on", sequence, sizeof(sequence) - 1);
	if (write_all(STDOUT_FILENO, sequence, sizeof(sequence) - 1) < 0)
		fatal("write terminal");
}

static void update_size(bool from_signal)
{
	struct winsize size = { 0 };
	if (ioctl(STDIN_FILENO, TIOCGWINSZ, &size) < 0)
		fatal("TIOCGWINSZ");
	editor.rows = size.ws_row ? size.ws_row : 24;
	editor.cols = size.ws_col ? size.ws_col : 80;
	backend->resize(editor.rows, editor.cols);
	if (from_signal)
		editor.resizes++;
	trace_event("%s TIOCGWINSZ: rows=%u cols=%u (drawing %dx%d)",
		    from_signal ? "SIGWINCH ->" : "START ->", size.ws_row,
		    size.ws_col, editor.rows, editor.cols);
}

static size_t line_start(size_t position)
{
	while (position && editor.text.data[position - 1] != '\n')
		position--;
	return position;
}

static size_t line_end(size_t position)
{
	while (position < editor.text.len && editor.text.data[position] != '\n')
		position++;
	return position;
}

/* 返回一个 UTF-8 码点的字节数；无效字节保留，并显示成 \xNN。 */
static size_t glyph(const char *data, size_t len, size_t column, int *width)
{
	unsigned char c = (unsigned char)*data;
	if (c < 32 || c == 127) {
		*width = c == '\t' ? 4 - (int)(column % 4) : 2;
		return 1;
	}
	mbstate_t state = { 0 };
	wchar_t wc;
	size_t n = mbrtowc(&wc, data, len, &state);
	if (n == (size_t)-1 || n == (size_t)-2 || !n) {
		*width = 4;
		return 1;
	}
	*width = wcwidth(wc);
	if (*width < 0)
		*width = 1;
	return n;
}

static size_t previous_char(size_t position)
{
	size_t p = line_start(position), previous = p;
	if (p == position)
		return position ? position - 1 : 0;
	while (p < position) {
		int width;
		previous = p;
		p += glyph(editor.text.data + p, position - p, 0, &width);
	}
	return previous;
}

static size_t next_char(size_t position)
{
	if (position >= editor.text.len)
		return position;
	int width;
	return position + glyph(editor.text.data + position,
				editor.text.len - position, 0, &width);
}

static size_t cursor_column(void)
{
	size_t column = 0, p = line_start(editor.cursor);
	while (p < editor.cursor) {
		int width;
		p += glyph(editor.text.data + p, editor.cursor - p, column,
			   &width);
		column += (size_t)width;
	}
	return column;
}

static void move_vertical(int direction)
{
	size_t target = cursor_column(), p = line_start(editor.cursor);
	if (direction < 0) {
		if (!p)
			return;
		p = line_start(p - 1);
	} else {
		p = line_end(p);
		if (p == editor.text.len)
			return;
		p++;
	}
	size_t column = 0;
	while (p < editor.text.len && editor.text.data[p] != '\n') {
		int width;
		size_t n = glyph(editor.text.data + p, editor.text.len - p,
				 column, &width);
		if (column + (size_t)width > target)
			break;
		column += (size_t)width;
		p += n;
	}
	editor.cursor = p;
}

/* 文件里的 ESC 等控制字节必须可视化，不能当成绘制命令发送。 */
static void draw_text(const char *data, size_t len, size_t left, size_t columns)
{
	size_t column = 0;
	for (size_t p = 0; p < len && column < left + columns;) {
		int width;
		size_t n = glyph(data + p, len - p, column, &width);
		size_t end = column + (size_t)width;
		unsigned char c = (unsigned char)data[p];
		if (column >= left && end <= left + columns) {
			if (c == '\t') {
				for (int i = 0; i < width; i++)
					emit(" ");
			} else if (c < 32 || c == 127) {
				char escaped[] = { '^',
						   c == 127 ? '?' :
							      (char)(c + 64) };
				backend->text(escaped, sizeof(escaped));
			} else if (width == 4 && n == 1 && c >= 128) {
				char escaped[5];
				snprintf(escaped, sizeof(escaped), "\\x%02x",
					 c);
				emit(escaped);
			} else {
				mbstate_t state = { 0 };
				wchar_t wc;
				(void)mbrtowc(&wc, data + p, n, &state);
				if (wcwidth(wc) < 0)
					emit("?");
				else
					backend->text(data + p, n);
			}
		} else if (end > left && column < left + columns) {
			/* 横向滚动切到宽字符中间时，用空格填充被裁剪的部分。 */
			size_t first = column < left ? left : column;
			size_t last = end > left + columns ? left + columns :
							     end;
			for (size_t i = first; i < last; i++)
				emit(" ");
		}
		column = end;
		p += n;
	}
}

static void cursor_to(int row, int column)
{
	backend->position(row, column);
}

static void draw_screen(void)
{
	size_t row = 0, column = cursor_column();
	for (size_t i = 0; i < editor.cursor; i++)
		row += editor.text.data[i] == '\n';
	editor.frame.len = 0;
	backend->begin();
	if (editor.rows < 3 || editor.cols < 12) {
		cursor_to(1, 1);
		backend->erase_screen();
		draw_text("Enlarge window", 14, 0, (size_t)(editor.cols - 1));
	} else {
		size_t height = (size_t)editor.rows - 2;
		size_t width =
			(size_t)editor.cols - 1; /* 留一列，避免自动折行。 */
		if (row < editor.top)
			editor.top = row;
		if (row >= editor.top + height)
			editor.top = row - height + 1;
		if (column < editor.left)
			editor.left = column;
		if (column >= editor.left + width)
			editor.left = column - width + 1;
		size_t p = 0;
		for (size_t i = 0; i < editor.top && p < editor.text.len; i++)
			p = line_end(p) + 1;
		for (size_t y = 0; y < height; y++) {
			cursor_to((int)y + 1, 1);
			backend->erase_line();
			if (p <= editor.text.len) {
				size_t end = line_end(p);
				draw_text(editor.text.data + p, end - p,
					  editor.left, width);
				p = end + 1;
			} else {
				backend->style(STYLE_CYAN);
				emit("~");
				backend->style(STYLE_NORMAL);
			}
		}
		char status[256];
		const char *mode = editor.mode == INSERT  ? "INSERT" :
				   editor.mode == COMMAND ? "COMMAND" :
							    "NORMAL";
		snprintf(
			status, sizeof(status),
			" %s %s | %s | %zu:%zu | %dx%d | winch=%u | ICANON=0 ECHO=0 ISIG=0",
			mode, editor.dirty ? "[+]" : "[-]", backend->name, row + 1, column + 1,
			editor.rows, editor.cols, editor.resizes);
		cursor_to(editor.rows - 1, 1);
		backend->style(STYLE_STATUS);
		backend->erase_line();
		draw_text(status, strlen(status), 0, width);
		backend->style(STYLE_NORMAL);
		cursor_to(editor.rows, 1);
		backend->erase_line();
		if (editor.mode == COMMAND) {
			emit(":");
			size_t visible = width - 1;
			size_t offset = editor.command_len > visible ?
						editor.command_len - visible :
						0;
			draw_text(editor.command + offset,
				  editor.command_len - offset, 0, visible);
			cursor_to(editor.rows,
				  (int)(editor.command_len - offset) + 2);
		} else {
			draw_text(editor.message, strlen(editor.message), 0,
				  width);
			cursor_to((int)(row - editor.top) + 1,
				  (int)(column - editor.left) + 1);
		}
	}
	backend->present(editor.mode == NORMAL);
}

static int read_byte(int timeout)
{
	struct pollfd fd = { .fd = STDIN_FILENO, .events = POLLIN };
	int ready = poll(&fd, 1, timeout);
	if (ready < 0) {
		if (errno == EINTR)
			return KEY_NONE;
		fatal("poll");
	}
	if (!ready)
		return KEY_NONE;
	unsigned char byte;
	ssize_t n = read(STDIN_FILENO, &byte, 1);
	if (n < 0 && errno == EINTR)
		return KEY_NONE;
	if (n < 0)
		fatal("read terminal");
	if (!n) {
		editor.quit = true;
		return KEY_NONE;
	}
	trace_bytes("RX", (const char *)&byte, 1);
	return byte;
}

static int read_key(void)
{
	int key = read_byte(100);
	if (key != 27)
		return key;
	/* ESC 单键与方向键共享前缀；短暂等待后续字节。 */
	int next = read_byte(50);
	if (next == KEY_NONE)
		return 27;
	if (next != '[' && next != 'O') {
		/* 快速输入 ESC 后再输入普通键，不能丢掉后者。 */
		editor.mode = NORMAL;
		editor.prefix = 0;
		return next;
	}
	char sequence[24];
	size_t len = 0;
	while (len + 1 < sizeof(sequence)) {
		next = read_byte(50);
		if (next == KEY_NONE)
			return KEY_NONE;
		sequence[len++] = (char)next;
		if (next >= 0x40 && next <= 0x7e)
			break;
	}
	sequence[len] = '\0';
	if (!strcmp(sequence, "A"))
		return KEY_UP;
	if (!strcmp(sequence, "B"))
		return KEY_DOWN;
	if (!strcmp(sequence, "C"))
		return KEY_RIGHT;
	if (!strcmp(sequence, "D"))
		return KEY_LEFT;
	if (!strcmp(sequence, "H") || !strcmp(sequence, "1~"))
		return KEY_HOME;
	if (!strcmp(sequence, "F") || !strcmp(sequence, "4~"))
		return KEY_END;
	if (!strcmp(sequence, "3~"))
		return KEY_DELETE;
	if (!strcmp(sequence, "200~"))
		return KEY_PASTE_BEGIN;
	if (!strcmp(sequence, "201~"))
		return KEY_PASTE_END;
	return KEY_NONE;
}

static void insert_byte(char byte)
{
	if (editor.text.len >= FILE_LIMIT) {
		message("File limit: 8 MiB");
		return;
	}
	reserve(&editor.text, editor.text.len + 2);
	memmove(editor.text.data + editor.cursor + 1,
		editor.text.data + editor.cursor,
		editor.text.len - editor.cursor);
	editor.text.data[editor.cursor++] = byte;
	editor.text.data[++editor.text.len] = '\0';
	editor.dirty = true;
}

static void delete_range(size_t start, size_t end)
{
	if (start == end)
		return;
	memmove(editor.text.data + start, editor.text.data + end,
		editor.text.len - end);
	editor.text.len -= end - start;
	editor.text.data[editor.text.len] = '\0';
	editor.cursor = start;
	editor.dirty = true;
}

static void load_file(const char *path)
{
	editor.path = strdup(path);
	if (!editor.path)
		fatal("strdup");
	int fd = open(path, O_RDONLY | O_NONBLOCK);
	if (fd < 0) {
		if (errno == ENOENT) {
			message("New file: %s", path);
			return;
		}
		fatal("open file");
	}
	struct stat st;
	if (fstat(fd, &st) < 0)
		fatal("fstat");
	if (!S_ISREG(st.st_mode) || st.st_size > FILE_LIMIT) {
		fprintf(stderr,
			"mini-vim: requires a regular file of at most 8 MiB\n");
		exit(EXIT_FAILURE);
	}
	char block[4096];
	ssize_t n;
	while ((n = read(fd, block, sizeof(block))) != 0) {
		if (n < 0) {
			if (errno == EINTR)
				continue;
			fatal("read file");
		}
		if (editor.text.len + (size_t)n > FILE_LIMIT) {
			errno = EFBIG;
			fatal("read file");
		}
		append(&editor.text, block, (size_t)n);
	}
	if (close(fd) < 0)
		fatal("close file");
}

/* 同目录临时文件 + rename，写失败不会先截断原文件。 */
static bool save_file(const char *path)
{
	if (!path || !*path) {
		message("No file name. Use :w filename");
		return false;
	}
	struct stat st;
	mode_t mode = 0600;
	if (lstat(path, &st) == 0) {
		if (!S_ISREG(st.st_mode) || st.st_nlink != 1) {
			message("Save refused: symlink, non-regular file or multiple hard links");
			return false;
		}
		if (access(path, W_OK) < 0) {
			message("Save failed: %s", strerror(errno));
			return false;
		}
		if (editor.trace) {
			struct stat trace_st;
			if (fstat(fileno(editor.trace), &trace_st) < 0)
				fatal("fstat trace");
			if (st.st_dev == trace_st.st_dev &&
			    st.st_ino == trace_st.st_ino) {
				message("Save refused: destination is the trace log");
				return false;
			}
		}
		mode = st.st_mode & 0777;
	} else if (errno != ENOENT) {
		message("Save failed: %s", strerror(errno));
		return false;
	}
	size_t len = strlen(path);
	char *temporary = malloc(len + sizeof(".mini-vim-XXXXXX"));
	char *new_path = strdup(path);
	if (!temporary || !new_path)
		fatal("allocate save path");
	snprintf(temporary, len + sizeof(".mini-vim-XXXXXX"),
		 "%s.mini-vim-XXXXXX", path);
	int fd = mkstemp(temporary);
	int error = fd < 0 ? errno : 0;
	if (fd >= 0) {
		if (write_all(fd, editor.text.data, editor.text.len) < 0 ||
		    fchmod(fd, mode) < 0 || fsync(fd) < 0)
			error = errno;
		if (close(fd) < 0 && !error)
			error = errno;
		if (!error && rename(temporary, path) < 0)
			error = errno;
		if (error)
			(void)unlink(temporary);
	}
	free(temporary);
	if (error) {
		message("Save failed: %s", strerror(error));
		free(new_path);
		return false;
	}
	free(editor.path);
	editor.path = new_path;
	editor.dirty = false;
	message("Written %zu bytes: %s", editor.text.len, editor.path);
	trace_event("SAVE %zu bytes", editor.text.len);
	return true;
}

static void run_command(void)
{
	const char *command = editor.command;
	editor.mode = NORMAL;
	if (!strcmp(command, "q!"))
		editor.quit = true;
	else if (!strcmp(command, "q")) {
		if (editor.dirty)
			message("Unsaved changes. Use :w, :wq or :q!");
		else
			editor.quit = true;
	} else if (!strcmp(command, "w") || !strcmp(command, "wq")) {
		if (save_file(editor.path) && command[1] == 'q')
			editor.quit = true;
	} else if (!strncmp(command, "w ", 2)) {
		const char *path = command + 2;
		while (*path == ' ')
			path++;
		(void)save_file(path);
	} else if (!strcmp(command, "help")) {
		message("i/a/o insert | Esc normal | hjkl/arrows | 0/$ gg/G | x dd | :w "
			":wq :q!");
	} else {
		message("Unknown command: %s", command);
	}
}

static void handle_key(int key)
{
	if (key == KEY_PASTE_BEGIN) {
		editor.paste = true;
		editor.mode = INSERT;
		return;
	}
	if (key == KEY_PASTE_END) {
		editor.paste = false;
		return;
	}
	if (editor.paste) {
		if (key == '\r' || key == '\n')
			insert_byte('\n');
		else if (key == '\t' || (key >= 32 && key < 256 && key != 127))
			insert_byte((char)key);
		return;
	}
	if (key == 27 || key == 3) {
		editor.mode = NORMAL;
		editor.prefix = 0;
		message(key == 3 ?
				"Ctrl-C = byte 0x03 (ISIG=0); handled by the editor" :
				"NORMAL");
		return;
	}
	if (editor.mode == COMMAND) {
		if (key == '\r' || key == '\n')
			run_command();
		else if (key == 127 || key == 8) {
			if (editor.command_len)
				editor.command[--editor.command_len] = '\0';
			else
				editor.mode = NORMAL;
		} else if (key >= 32 && key < 127 &&
			   editor.command_len + 1 < sizeof(editor.command)) {
			editor.command[editor.command_len++] = (char)key;
			editor.command[editor.command_len] = '\0';
		}
		return;
	}
	if (key == KEY_UP || (editor.mode == NORMAL && key == 'k'))
		move_vertical(-1);
	else if (key == KEY_DOWN || (editor.mode == NORMAL && key == 'j'))
		move_vertical(1);
	else if (key == KEY_LEFT || (editor.mode == NORMAL && key == 'h')) {
		if (editor.cursor > line_start(editor.cursor))
			editor.cursor = previous_char(editor.cursor);
	} else if (key == KEY_RIGHT || (editor.mode == NORMAL && key == 'l')) {
		if (editor.cursor < line_end(editor.cursor))
			editor.cursor = next_char(editor.cursor);
	} else if (key == KEY_HOME || (editor.mode == NORMAL && key == '0'))
		editor.cursor = line_start(editor.cursor);
	else if (key == KEY_END || (editor.mode == NORMAL && key == '$'))
		editor.cursor = line_end(editor.cursor);
	else if (key == KEY_DELETE || (editor.mode == NORMAL && key == 'x')) {
		if (editor.cursor < line_end(editor.cursor))
			delete_range(editor.cursor, next_char(editor.cursor));
	} else if (editor.mode == INSERT) {
		if (key == 127 || key == 8)
			delete_range(previous_char(editor.cursor),
				     editor.cursor);
		else if (key == '\r' || key == '\n')
			insert_byte('\n');
		else if (key == '\t' || (key >= 32 && key < 256))
			insert_byte((char)key);
	} else if (key == ':') {
		editor.command_len = 0;
		editor.command[0] = '\0';
		editor.mode = COMMAND;
	} else if (key == 'i' || key == 'a' || key == 'o') {
		if (key == 'a' && editor.cursor < line_end(editor.cursor))
			editor.cursor = next_char(editor.cursor);
		if (key == 'o') {
			editor.cursor = line_end(editor.cursor);
			insert_byte('\n');
		}
		editor.mode = INSERT;
	} else if (key == 'G') {
		editor.cursor = line_start(editor.text.len);
	} else if (key == 'g' && editor.prefix == 'g') {
		editor.cursor = 0;
	} else if (key == 'd' && editor.prefix == 'd') {
		size_t start = line_start(editor.cursor),
		       end = line_end(editor.cursor);
		if (end < editor.text.len)
			end++;
		else if (start)
			start--;
		delete_range(start, end);
		editor.cursor = line_start(editor.cursor);
	} else if (key == 'g' || key == 'd') {
		editor.prefix = key;
		return;
	}
	editor.prefix = 0;
}

int main(int argc, char **argv)
{
	(void)setlocale(LC_CTYPE, "");
	const char *path = NULL, *trace_path = NULL;
	for (int i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "--backend") && i + 1 < argc) {
			const char *name = argv[++i];
			if (!strcmp(name, "ansi"))
				backend = &backend_ansi;
#ifndef MINI_VIM_ANSI_ONLY
			else if (!strcmp(name, "terminfo"))
				backend = &backend_terminfo;
			else if (!strcmp(name, "curses"))
				backend = &backend_curses;
#endif
			else {
				fprintf(stderr, "mini-vim: unavailable backend: %s\n", name);
				return EXIT_FAILURE;
			}
		} else if (!strcmp(argv[i], "--trace") && i + 1 < argc && !trace_path)
			trace_path = argv[++i];
		else if (!strcmp(argv[i], "--help")) {
			puts("Usage: mini-vim.out [--backend ansi|terminfo|curses] "
			     "[--trace logfile] [file]\n"
			     "i/a/o: insert; Esc: normal; hjkl/arrows: move; :help: keys; :q!: "
			     "quit");
			return 0;
		} else if (!path && strncmp(argv[i], "--", 2))
			path = argv[i];
		else {
			fprintf(stderr,
				"Usage: mini-vim.out [--backend ansi|terminfo|curses] "
				"[--trace logfile] [file]\n");
			return EXIT_FAILURE;
		}
	}
	append(&editor.text, "", 0);
	message("i: insert | :help: keys | :q!: quit | resize window to see SIGWINCH");
	if (path)
		load_file(path);
	if (trace_path) {
		/* 拒绝覆盖任何已有文件，也防止日志与编辑文件相同。 */
		if (path && !strcmp(path, trace_path)) {
			fprintf(stderr,
				"mini-vim: trace log and edited file must differ\n");
			return EXIT_FAILURE;
		}
		int fd = open(trace_path, O_WRONLY | O_CREAT | O_EXCL, 0600);
		if (fd < 0)
			fatal("create trace (choose a new log path)");
		editor.trace = fdopen(fd, "w");
		if (!editor.trace)
			fatal("fdopen trace");
	}
	terminal_start();
	update_size(false);
	bool redraw = true;
	while (!editor.quit && !stopped) {
		if (resized) {
			resized = 0;
			update_size(true);
			redraw = true;
		}
		if (redraw) {
			draw_screen();
			redraw = false;
		}
		int key = read_key();
		if (key != KEY_NONE) {
			handle_key(key);
			redraw = true;
		}
	}
	if (stopped)
		trace_event("EXIT on signal %d", (int)stopped);
	terminal_restore();
	if (editor.trace)
		fclose(editor.trace);
	free(editor.text.data);
	free(editor.frame.data);
	free(editor.path);
	return stopped ? 128 + (int)stopped : 0;
}
