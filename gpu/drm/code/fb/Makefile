PKG_CONFIG ?= $(shell if [ -n "$$IN_NIX_SHELL" ]; then echo pkg-config; elif command -v pkgconf >/dev/null 2>&1; then echo pkgconf; else echo pkg-config; fi)
CC = gcc
PKGS = libdrm egl glesv2 wayland-client x11
CFLAGS = -Wall -Wextra -std=c99 -g -DWITH_X11 $(shell $(PKG_CONFIG) --cflags $(PKGS) 2>/dev/null || echo "")
LIBS = $(shell $(PKG_CONFIG) --libs $(PKGS) 2>/dev/null || echo "-ldrm -lEGL -lGLESv2 -lwayland-client -lX11")

# VNC red triangle programs
RENDER_PROG = render_triangle.out
RENDER_OBJ = render_triangle.o
FB_RENDER_PROG = draw_triangle_fb.out
FB_RENDER_OBJ = framebuffer.o

all: $(RENDER_PROG) $(FB_RENDER_PROG)

$(RENDER_PROG): $(RENDER_OBJ)
	$(CC) -o $@ $< $(LIBS)

$(FB_RENDER_PROG): $(FB_RENDER_OBJ)
	$(CC) -o $@ $<

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(RENDER_OBJ) $(RENDER_PROG) $(FB_RENDER_OBJ) $(FB_RENDER_PROG)

.PHONY: all clean install-deps test
