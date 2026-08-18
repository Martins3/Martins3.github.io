PKG_CONFIG ?= $(shell if [ -n "$$IN_NIX_SHELL" ]; then echo pkg-config; elif command -v pkgconf >/dev/null 2>&1; then echo pkgconf; else echo pkg-config; fi)
WAYLAND_FLAGS = $(shell $(PKG_CONFIG) --cflags --libs wayland-client wayland-egl egl glesv2)
WAYLAND_PROTOCOLS_DIR = $(shell $(PKG_CONFIG) --variable=pkgdatadir wayland-protocols)
WAYLAND_SCANNER ?= $(shell command -v wayland-scanner)

XDG_SHELL_PROTOCOL = $(WAYLAND_PROTOCOLS_DIR)/stable/xdg-shell/xdg-shell.xml

HEADERS=xdg-shell-client-protocol.h
SOURCES=xdg-shell-protocol.c
TARGETS=window.out window_egl.out demo2.out

all: $(TARGETS)

window.out: window.c $(HEADERS) $(SOURCES)
	gcc -o $@ $< $(SOURCES) -I. $(WAYLAND_FLAGS)

window_egl.out: window_egl.c $(HEADERS) $(SOURCES)
	gcc -o $@ $< $(SOURCES) -I. $(WAYLAND_FLAGS)

demo2.out: demo2.c wayland-logo.h
	gcc -g -o $@ $<

$(HEADERS):
	$(WAYLAND_SCANNER) client-header $(XDG_SHELL_PROTOCOL) $@

$(SOURCES):
	$(WAYLAND_SCANNER) private-code $(XDG_SHELL_PROTOCOL) $@

clean:
	rm -rf $(TARGETS) $(HEADERS) $(SOURCES)
