/*
 * 展示一个最小的例子，通过 g_source_add_poll 来监听 stdin
 */
#include <glib.h>
#include <stdio.h>

gboolean aio_ctx_prepare(GSource *source, gint *timeout_)
{
	g_print("%s called\n", __FUNCTION__);
	*timeout_ = -1;
	return FALSE;
}
gboolean aio_ctx_check(GSource *source)
{
	g_print("%s called\n", __FUNCTION__);
	return TRUE;
}
#define MAX_LINE 1000
gboolean aio_ctx_dispatch(GSource *source, GSourceFunc callback,
			  gpointer user_data)
{
	g_print("%s called\n", __FUNCTION__);

	char line[MAX_LINE];
	fgets(line, MAX_LINE, stdin);
	g_print("get input [%s]\n", line);
	return TRUE;
}
void aio_ctx_finalize(GSource *source)
{
	g_print("%s called\n", __FUNCTION__);
}

static GSourceFuncs aio_source_funcs = { aio_ctx_prepare, aio_ctx_check,
					 aio_ctx_dispatch, aio_ctx_finalize };

typedef struct {
	GSource source;
	GPollFD pfd;
} AioContext;

void non_default_g_source()
{
	AioContext *ctx = (AioContext *)g_source_new(&aio_source_funcs,
						     sizeof(AioContext));

	// 监听标准输出
	ctx->pfd.events = G_IO_IN;
	ctx->pfd.fd = 0;

	g_source_add_poll(&ctx->source, &ctx->pfd);

	GMainLoop *loop1 = g_main_loop_new(NULL, FALSE);
	g_source_attach(&ctx->source, NULL);

	g_print("run loop1\n");
	g_main_loop_run(loop1);
}

int main()
{
	non_default_g_source();
	return 0;
}
