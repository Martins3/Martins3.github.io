#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>

#include "xdg-shell-client-protocol.h"

static struct wl_compositor *compositor = NULL;
struct xdg_wm_base *xdg_shell = NULL;
struct wl_region *region = NULL;
struct wl_surface *surface = NULL;

const int WIDTH = 480;
const int HEIGHT = 360;

static void handle_configure(void *data, struct xdg_surface *surface,
			     uint32_t serial)
{
	xdg_surface_ack_configure(surface, serial);
}

static const struct xdg_surface_listener surface_listener = {
	.configure = handle_configure
};

static void global_registry_handler(void *data, struct wl_registry *registry,
				    uint32_t id, const char *interface,
				    uint32_t version)
{
	printf("Got a registry event for %s id %d\n", interface, id);
	if (strcmp(interface, "wl_compositor") == 0) {
		compositor = wl_registry_bind(
			registry, id, &wl_compositor_interface, version);
	} else if (strcmp(interface, xdg_wm_base_interface.name) == 0) {
		xdg_shell = wl_registry_bind(registry, id,
					     &xdg_wm_base_interface, version);
	}
}

static void global_registry_remover(void *data, struct wl_registry *registry,
				    uint32_t id)
{
	printf("Got a registry losing event for %d\n", id);
}

static const struct wl_registry_listener registry_listener = {
	global_registry_handler,
	global_registry_remover,
};

int main(int argc, char **argv)
{
	fprintf(stderr, "XDG_RUNTIME_DIR= %s\n", getenv("XDG_RUNTIME_DIR"));

	struct wl_display *display = display = wl_display_connect(NULL);
	if (display == NULL) {
		fprintf(stderr, "Can't connect to display\n");
		exit(1);
	}
	printf("connected to display\n");

	struct wl_registry *registry = wl_display_get_registry(display);
	if (!registry) {
		printf("Cannot get registry.\n");
		exit(1);
	}

	wl_registry_add_listener(registry, &registry_listener, NULL);

	wl_display_dispatch(display);
	wl_display_roundtrip(display);

	if (compositor == NULL) {
		fprintf(stderr, "Can't find compositor\n");
		exit(1);
	} else {
		fprintf(stderr, "Found compositor\n");
	}

	surface = wl_compositor_create_surface(compositor);
	if (surface == NULL) {
		fprintf(stderr, "Can't create surface\n");
		exit(1);
	} else {
		fprintf(stderr, "Created surface\n");
	}

	if (!xdg_shell) {
		fprintf(stderr, "Cannot bind to shell\n");
		exit(1);
	} else {
		fprintf(stderr, "Found valid shell.\n");
	}
	struct xdg_surface *shell_surface =
		xdg_wm_base_get_xdg_surface(xdg_shell, surface);
	if (shell_surface == NULL) {
		fprintf(stderr, "Can't create shell surface\n");
		exit(1);
	} else {
		fprintf(stderr, "Created shell surface\n");
	}
	// 窗口处理
	struct xdg_toplevel *toplevel = xdg_surface_get_toplevel(shell_surface);
	xdg_surface_add_listener(shell_surface, &surface_listener, NULL);

	region = wl_compositor_create_region(compositor);
	wl_region_add(region, 0, 0, WIDTH, HEIGHT);
	wl_surface_set_opaque_region(surface, region);

	// 此处是窗口显示
	// init egl
	EGLint major, minor, config_count, tmp_n;
	EGLint config_attribs[] = { EGL_SURFACE_TYPE,
				    EGL_WINDOW_BIT,
				    EGL_RED_SIZE,
				    8,
				    EGL_GREEN_SIZE,
				    8,
				    EGL_BLUE_SIZE,
				    8,
				    EGL_RENDERABLE_TYPE,
				    EGL_OPENGL_ES2_BIT,
				    EGL_NONE };
	EGLint context_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };

	EGLConfig egl_config;

	EGLDisplay egl_display = eglGetDisplay((EGLNativeDisplayType)display);
	eglInitialize(egl_display, &major, &minor);
	printf("EGL major: %d, minor %d\n", major, minor);
	eglGetConfigs(egl_display, 0, 0, &config_count);
	printf("EGL has %d configs\n", config_count);
	eglChooseConfig(egl_display, config_attribs, &(egl_config), 1, &tmp_n);
	EGLContext egl_context = eglCreateContext(
		egl_display, egl_config, EGL_NO_CONTEXT, context_attribs);

	// init window
	struct wl_egl_window *egl_window =
		wl_egl_window_create(surface, WIDTH, HEIGHT);
	EGLSurface egl_surface =
		eglCreateWindowSurface(egl_display, egl_config, egl_window, 0);
	eglMakeCurrent(egl_display, egl_surface, egl_surface, egl_context);

	/* TODO
	 * 1. 存在这个警告: libEGL warning: egl: failed to create dri2 screen
	 * 2. 程序会报告 not responding
	 * */

	glClearColor(0.5f, 0.5f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	eglSwapBuffers(egl_display, egl_surface);

	// 窗口循环直到关闭
	while (wl_display_dispatch(display) != -1) {
		;
	}

	xdg_surface_destroy(shell_surface);
	xdg_wm_base_destroy(xdg_shell);
	wl_surface_destroy(surface);
	wl_compositor_destroy(compositor);
	wl_registry_destroy(registry);
	wl_display_disconnect(display);
	printf("disconnected from display\n");

	exit(0);
}
