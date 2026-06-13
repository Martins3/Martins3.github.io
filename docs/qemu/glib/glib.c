#include <glib.h>
#include <stdio.h>

gboolean callback(gpointer data) {
  static guint16 i = 0;
  g_print("Iter=%" G_GUINT16_FORMAT "\n", i++);
  if (i % 3 == 0) {
    g_print("try to stop main loop\n");
    g_main_loop_quit((GMainLoop *)data);
  }
  return TRUE;
}

void two_main_loop() {
  GMainContext *con1 = NULL;
  GMainContext *con2 = NULL;
  GMainLoop *loop1 = NULL;
  GMainLoop *loop2 = NULL;
  GSource *source1 = NULL;
  GSource *source2 = NULL;

  con1 = g_main_context_new();
  con2 = g_main_context_new();

  loop1 = g_main_loop_new(con1, FALSE);
  loop2 = g_main_loop_new(con2, FALSE);

  source1 = g_timeout_source_new_seconds(1);
  g_source_set_callback(source1, callback, loop1, NULL);
  g_source_attach(source1, con1);

  source2 = g_timeout_source_new_seconds(1);
  g_source_set_callback(source2, callback, loop2, NULL);
  g_source_attach(source2, con2);

  g_print("run loop1\n");
  g_main_loop_run(loop1);

  g_main_loop_unref(loop1);
  loop1 = NULL;

  g_print("run loop2\n");

  g_main_loop_run(loop2);
}

void one_loop() {
  GMainContext *con1 = NULL;
  GMainLoop *loop1 = NULL;
  GSource *source1 = NULL;

  con1 = g_main_context_new();

  loop1 = g_main_loop_new(con1, FALSE);

  source1 = g_timeout_source_new_seconds(1);
  g_source_set_callback(source1, callback, loop1, NULL);
  g_source_attach(source1, con1);

  g_print("run loop1\n");
  g_main_loop_run(loop1);
}

/**
 * if context is not specified, defautl context will be used
 */
void defautl_context() {
  GMainLoop *loop1 = NULL;
  GSource *source1 = NULL;

  loop1 = g_main_loop_new(NULL, FALSE);

  source1 = g_timeout_source_new_seconds(1);
  g_source_set_callback(source1, callback, loop1, NULL);
  g_source_attach(source1, NULL);

  g_print("run loop1\n");
  g_main_loop_run(loop1);
}

gboolean aio_ctx_prepare(GSource *source, gint *timeout_) {
  g_print("%s called\n", __FUNCTION__);
  *timeout_ = -1;
  return FALSE;
}
gboolean aio_ctx_check(GSource *source) {
  g_print("%s called\n", __FUNCTION__);
  return TRUE;
}
#define MAX_LINE 1000
gboolean aio_ctx_dispatch(GSource *source, GSourceFunc callback,
                          gpointer user_data) {
  g_print("%s called\n", __FUNCTION__);

  char line[MAX_LINE];
  fgets(line, MAX_LINE, stdin);
  g_print("get input [%s]\n", line);
  return TRUE;
}
void aio_ctx_finalize(GSource *source) { g_print("%s called\n", __FUNCTION__); }

static GSourceFuncs aio_source_funcs = {aio_ctx_prepare, aio_ctx_check,
                                        aio_ctx_dispatch, aio_ctx_finalize};

typedef struct {
  GSource source;
  GPollFD pfd;
} AioContext;

void non_default_g_source() {
  AioContext *ctx =
      (AioContext *)g_source_new(&aio_source_funcs, sizeof(AioContext));

  ctx->pfd.events = G_IO_IN;
  ctx->pfd.fd = 0; // standard input

  g_source_add_poll(&ctx->source, &ctx->pfd);

  GMainLoop *loop1 = g_main_loop_new(NULL, FALSE);
  g_source_attach(&ctx->source, NULL);

  g_print("run loop1\n");
  g_main_loop_run(loop1);
}

int main() {
  non_default_g_source();
  return 0;
}
