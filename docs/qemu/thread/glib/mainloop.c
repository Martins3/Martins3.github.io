#include <glib.h>
#include <stdio.h>
gboolean callback(gpointer data)
{
	static guint16 i = 0;
	g_print("Iter=%" G_GUINT16_FORMAT "\n", i++);
	if (i % 3 == 0) {
		g_print("try to stop main loop\n");
		g_main_loop_quit((GMainLoop *)data);
	}
	return TRUE;
}

void two_loop()
{
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

void one_loop()
{
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
void defautl_context()
{
	GMainLoop *loop1 = NULL;
	GSource *source1 = NULL;
	loop1 = g_main_loop_new(NULL, FALSE);
	source1 = g_timeout_source_new_seconds(1);
	g_source_set_callback(source1, callback, loop1, NULL);
	g_source_attach(source1, NULL);
	g_print("run loop1\n");
	g_main_loop_run(loop1);
}

static inline void help()
{
	printf("usage:\n");
	printf("1 : default\n");
	printf("2 : one_loop\n");
	printf("3 : two_loop\n");
}

int main(int argc, char *argv[])
{
	if (argc == 2) {
		char *m;
		int a = strtol(argv[1], &m, 10);
		switch (a) {
		case 0:
			defautl_context();
			break;
		case 1:
			one_loop();
			break;
		case 2:
			two_loop();
			break;
		default:
			help();
		}
	} else
		help();
	return EXIT_SUCCESS;
}
