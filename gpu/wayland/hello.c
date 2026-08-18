#include <wayland-server.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>
#include <wayland-util.h>

#include <wayland-client.h>
#include <stdio.h>

int main()
{
	/* 有趣，如果还没有 login 状态，虽然可以 ssh 进去，wl connect 会失败
	 *
	 */
	struct wl_display *display = wl_display_connect(0);

	if (!display) {
		fprintf(stderr, "Unable to connect to wayland compositor\n");
		return -1;
	}

	wl_display_disconnect(display);

	return 0;
}
