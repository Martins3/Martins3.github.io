CC ?= cc
CFLAGS ?= -O2 -g
CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -Werror
PKG_CONFIG ?= pkg-config
CURSES_CFLAGS = $(shell $(PKG_CONFIG) --cflags-only-I ncursesw)
CURSES_LIBS = $(shell $(PKG_CONFIG) --libs ncursesw tinfo)
SOURCES = mini-vim.c backend-ansi.c backend-terminfo.c backend-curses.c

.PHONY: all check
all: mini-vim.out

mini-vim.out: $(SOURCES) backend.h
	$(CC) $(CPPFLAGS) $(CURSES_CFLAGS) $(CFLAGS) $(SOURCES) $(LDFLAGS) $(CURSES_LIBS) $(LDLIBS) -o $@

# Keep the original dependency-free experiment available.
mini-vim-ansi.out: mini-vim.c backend-ansi.c backend.h
	$(CC) $(CPPFLAGS) $(CFLAGS) -DMINI_VIM_ANSI_ONLY mini-vim.c backend-ansi.c $(LDFLAGS) $(LDLIBS) -o $@

check: mini-vim.out
	nvim --headless -u NONE -l test-backends.lua
