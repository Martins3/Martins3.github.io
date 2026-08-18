PKG_CONFIG ?= $(shell if [ -n "$$IN_NIX_SHELL" ]; then echo pkg-config; elif command -v pkgconf >/dev/null 2>&1; then echo pkgconf; else echo pkg-config; fi)
WAYLAND_PKGS = wayland-client wayland-server wayland-cursor wayland-egl

SRCS = $(wildcard ./*.c)
OUTS = $(SRCS:.c=.out)

CPPFLAGS += $(shell $(PKG_CONFIG) --cflags $(WAYLAND_PKGS))
CFLAGS += -Wall
LDLIBS += $(shell $(PKG_CONFIG) --libs $(WAYLAND_PKGS))

all: $(OUTS)

%.out: %.o
	$(CC) $(LDFLAGS) -o $@ $< $(LDLIBS)

clean:
	rm -f ./*.o $(OUTS)
