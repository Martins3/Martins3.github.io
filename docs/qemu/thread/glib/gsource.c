/*
 * 展示 GMainContext、GSource 和 fd 之间的关系：
 *
 *   一个 GMainContext
 *     |-- source-a
 *     |     |-- stdin
 *     |     `-- pipe-a1
 *     `-- source-b
 *           |-- pipe-b0
 *           `-- pipe-b1
 *
 * stdin 和每个 pipe 的读端通过 g_source_add_poll() 添加到对应的 GSource。
 * 程序向三个 pipe 写入消息后启动 GMainLoop，以便从输出中观察 GLib 如何
 * 检查并分发多个 GSource，以及一个 GSource 如何处理多个 fd。
 */
#include <errno.h>
#include <glib.h>
#include <string.h>
#include <unistd.h>

#define SOURCE_COUNT 2
#define FDS_PER_SOURCE 2
#define BUFFER_SIZE 128

typedef struct {
	/* GSource 必须是第一个字段，以便在回调中安全地向下转换。 */
	GSource source;
	const gchar *name;
	GPollFD poll_fds[FDS_PER_SOURCE];
	gboolean handled[FDS_PER_SOURCE];
	guint handled_count;
	guint *total_handled;
	GMainLoop *loop;
} MultiFdSource;

static gboolean multi_fd_prepare(GSource *source, gint *timeout_ms)
{
	MultiFdSource *multi_source = (MultiFdSource *)source;

	g_print("prepare  source=%s\n", multi_source->name);
	*timeout_ms = -1;
	return FALSE;
}

static gboolean multi_fd_check(GSource *source)
{
	MultiFdSource *multi_source = (MultiFdSource *)source;
	gboolean ready = FALSE;
	guint i;

	for (i = 0; i < FDS_PER_SOURCE; i++) {
		GPollFD *poll_fd = &multi_source->poll_fds[i];

		if (!multi_source->handled[i] &&
		    (poll_fd->revents & (G_IO_IN | G_IO_HUP | G_IO_ERR))) {
			g_print("check    source=%s fd=%d revents=0x%x ready\n",
				multi_source->name, poll_fd->fd,
				poll_fd->revents);
			ready = TRUE;
		}
	}

	return ready;
}

static gboolean multi_fd_dispatch(GSource *source, GSourceFunc callback,
				  gpointer user_data)
{
	MultiFdSource *multi_source = (MultiFdSource *)source;
	guint i;

	(void)callback;
	(void)user_data;

	for (i = 0; i < FDS_PER_SOURCE; i++) {
		GPollFD *poll_fd = &multi_source->poll_fds[i];
		char buffer[BUFFER_SIZE];
		ssize_t bytes;

		if (multi_source->handled[i] || !(poll_fd->revents & G_IO_IN))
			continue;

		do {
			bytes = read(poll_fd->fd, buffer, sizeof(buffer) - 1);
		} while (bytes < 0 && errno == EINTR);

		if (bytes < 0) {
			g_warning("read fd %d failed: %s", poll_fd->fd,
				  g_strerror(errno));
			continue;
		}
		if (bytes == 0)
			continue;

		buffer[bytes] = '\0';
		g_strchomp(buffer);
		multi_source->handled[i] = TRUE;
		multi_source->handled_count++;
		(*multi_source->total_handled)++;
		g_print("dispatch source=%s fd[%u]=%d message=\"%s\"\n",
			multi_source->name, i, poll_fd->fd, buffer);
	}

	if (*multi_source->total_handled == SOURCE_COUNT * FDS_PER_SOURCE)
		g_main_loop_quit(multi_source->loop);

	/* 两个 fd 都处理完后，将当前 source 从 context 中移除。 */
	return multi_source->handled_count < FDS_PER_SOURCE;
}

static void multi_fd_finalize(GSource *source)
{
	MultiFdSource *multi_source = (MultiFdSource *)source;

	g_print("finalize source=%s\n", multi_source->name);
}

static GSourceFuncs multi_fd_source_funcs = {
	.prepare = multi_fd_prepare,
	.check = multi_fd_check,
	.dispatch = multi_fd_dispatch,
	.finalize = multi_fd_finalize,
};

static MultiFdSource *multi_fd_source_new(const gchar *name,
					  const int read_fds[FDS_PER_SOURCE],
					  guint *total_handled,
					  GMainLoop *loop)
{
	MultiFdSource *multi_source;
	guint i;

	multi_source = (MultiFdSource *)g_source_new(&multi_fd_source_funcs,
						       sizeof(*multi_source));
	multi_source->name = name;
	multi_source->total_handled = total_handled;
	multi_source->loop = loop;
	g_source_set_name(&multi_source->source, name);

	for (i = 0; i < FDS_PER_SOURCE; i++) {
		multi_source->poll_fds[i].fd = read_fds[i];
		multi_source->poll_fds[i].events = G_IO_IN;
		multi_source->poll_fds[i].revents = 0;
		g_source_add_poll(&multi_source->source,
				  &multi_source->poll_fds[i]);
	}

	return multi_source;
}

static void write_message(int fd, const gchar *message)
{
	size_t length = strlen(message);
	ssize_t written;

	do {
		written = write(fd, message, length);
	} while (written < 0 && errno == EINTR);

	if (written < 0)
		g_error("write fd %d failed: %s", fd, g_strerror(errno));
	if ((size_t)written != length)
		g_error("short write to fd %d: %zd/%zu", fd, written, length);
}

static void multiple_sources_with_multiple_fds(void)
{
	static const gchar *source_names[SOURCE_COUNT] = {
		"source-a",
		"source-b",
	};
	static const gchar *messages[SOURCE_COUNT][FDS_PER_SOURCE] = {
		{ NULL, "message from a1" },
		{ "message from b0", "message from b1" },
	};
	GMainContext *context;
	GMainLoop *loop;
	MultiFdSource *sources[SOURCE_COUNT] = { NULL };
	int pipes[SOURCE_COUNT][FDS_PER_SOURCE][2];
	guint total_handled = 0;
	guint i;
	guint j;

	context = g_main_context_new();
	loop = g_main_loop_new(context, FALSE);

	for (i = 0; i < SOURCE_COUNT; i++) {
		int read_fds[FDS_PER_SOURCE];

		for (j = 0; j < FDS_PER_SOURCE; j++) {
			if (i == 0 && j == 0) {
				pipes[i][j][0] = STDIN_FILENO;
				pipes[i][j][1] = -1;
				read_fds[j] = STDIN_FILENO;
				continue;
			}

			if (pipe(pipes[i][j]) < 0)
				g_error("pipe failed: %s", g_strerror(errno));
			read_fds[j] = pipes[i][j][0];
		}

		sources[i] = multi_fd_source_new(source_names[i], read_fds,
						 &total_handled, loop);
		g_source_attach(&sources[i]->source, context);

		g_print("attach   context=%p source=%s(%p) fds=[%d, %d]\n",
			context, source_names[i], sources[i], read_fds[0],
			read_fds[1]);
	}

	for (i = 0; i < SOURCE_COUNT; i++) {
		for (j = 0; j < FDS_PER_SOURCE; j++) {
			if (pipes[i][j][1] >= 0)
				write_message(pipes[i][j][1], messages[i][j]);
		}
	}

	g_print("run      one context, %d sources, %d fds per source\n",
		SOURCE_COUNT, FDS_PER_SOURCE);
	g_main_loop_run(loop);
	g_print("done     handled=%u/%d fd events\n", total_handled,
		SOURCE_COUNT * FDS_PER_SOURCE);

	for (i = 0; i < SOURCE_COUNT; i++) {
		g_source_destroy(&sources[i]->source);
		g_source_unref(&sources[i]->source);
		for (j = 0; j < FDS_PER_SOURCE; j++) {
			if (pipes[i][j][0] != STDIN_FILENO)
				close(pipes[i][j][0]);
			if (pipes[i][j][1] >= 0)
				close(pipes[i][j][1]);
		}
	}

	g_main_loop_unref(loop);
	g_main_context_unref(context);
}

int main(void)
{
	multiple_sources_with_multiple_fds();
	return 0;
}
