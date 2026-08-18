#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <wayland-client.h>
#include <wayland-version.h>

/**
 * announce global object
 *
 * Notify the client of global objects.
 *
 * The event notifies the client that a global object with the
 * given name is now available, and it implements the given version
 * of the given interface.
 * @param name numeric name of the global object
 * @param interface interface implemented by the object
 * @param version interface version
 */
static void on_global_added(void *data, struct wl_registry *wl_registry,
			    uint32_t name, const char *interface,
			    uint32_t version)
{
	(void)data;
	(void)wl_registry;
	fprintf(stderr, "☀️ Global added: %s, v %d (name %d)\n", interface,
		version, name);
}

/**
 * announce removal of global object
 *
 * Notify the client of removed global objects.
 *
 * This event notifies the client that the global identified by
 * name is no longer available. If the client bound to the global
 * using the bind request, the client should now destroy that
 * object.
 *
 * The object remains valid and requests to the object will be
 * ignored until the client destroys it, to avoid races between the
 * global going away and a client sending a request to it.
 * @param name numeric name of the global object
 */
static void on_global_removed(void *data, struct wl_registry *wl_registry,
			      uint32_t name)
{
	(void)data;
	(void)wl_registry;
	fprintf(stderr, "🌙 Global removed: name: %d\n", name);
}

static struct wl_registry_listener s_registryListener = {
	.global = on_global_added,
	.global_remove = on_global_removed
};

int main(int argc, char **argv)
{
	(void)argc;
	(void)argv;

	fprintf(stderr, "XDG_RUNTIME_DIR= %s\n", getenv("XDG_RUNTIME_DIR"));

	struct wl_display *disp = wl_display_connect(NULL);
	if (!disp) {
		fprintf(stderr, "Failed to connect to wayland display!\n");
		return 1;
	}
	fprintf(stderr, "Connect OK!\n");

	struct wl_registry *reg = wl_display_get_registry(disp);
	if (!reg) {
		fprintf(stderr, "Failed to get registry!\n");
	}

	fprintf(stderr, "Got registry OK!\n");

	wl_registry_add_listener(reg, &s_registryListener, NULL);

	wl_display_dispatch(disp);
	wl_display_roundtrip(disp);

	fprintf(stderr, "Nothing to display.\n");

	wl_registry_destroy(reg);
	wl_display_disconnect(disp);
	sleep(1);

	return 0;
}
