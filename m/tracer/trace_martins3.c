
/*
 *
 * cat available_tracers
   ls
   echo mytracer > current_tracer
   cat trace
 */
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/irqflags.h>
#include <linux/debugfs.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/ftrace.h>
#include <linux/trace.h>
#include <linux/trace_events.h>
#include <trace/events/sched.h>

#include "trace.h"

/* Define tracer options */
#define TRACE_MTRACER_OPT_CLASSIC 0x1

static struct tracer_opt my_tracer_opts[] = {
	{ TRACER_OPT(mtracer_classic, TRACE_MTRACER_OPT_CLASSIC) },
	{}
};

static struct tracer_flags my_tracer_flags = {
	.val = 0,
	.opts = my_tracer_opts,
};

static struct trace_array *m_tr;
static bool my_tracer_enabled __read_mostly = false;

/* Trace event structure */
#define TRACE_MTRACER 0x2001

/* Tracer function implementations */
static void my_tracer_start(struct trace_array *tr)
{
	my_tracer_enabled = true;
	pr_info("my_tracer: tracing started\n");
}

static int my_tracer_init(struct trace_array *tr)
{
	m_tr = tr;
	my_tracer_start(tr);
	pr_info("my_tracer: initialized\n");
	return 0;
}

static void my_tracer_stop(struct trace_array *tr)
{
	my_tracer_enabled = false;
	pr_info("my_tracer: tracing stopped\n");
}

static void my_tracer_reset(struct trace_array *tr)
{
	my_tracer_stop(tr);
	pr_info("my_tracer: reset\n");
}

static void my_tracer_print_header(struct seq_file *m)
{
	seq_puts(m, "# My Tracer Output\n");
	seq_puts(m, "# =================\n");
}

static enum print_line_t my_tracer_print_line(struct trace_iterator *iter)
{
	/* For this demo, just output a simple trace line */
	trace_seq_printf(&iter->seq, "my_tracer: event traced\n");
	return trace_handle_return(&iter->seq);
}

static int my_tracer_set_flag(struct trace_array *tr, u32 old_flags, u32 bit,
			      int set)
{
	return 0;
}

/* The main tracer structure */
static struct tracer my_tracer __read_mostly = {
	.name = "mytracer",
	.init = my_tracer_init,
	.reset = my_tracer_reset,
	.start = my_tracer_start,
	.stop = my_tracer_stop,
	.print_header = my_tracer_print_header,
	.print_line = my_tracer_print_line,
	.flags = &my_tracer_flags,
	.set_flag = my_tracer_set_flag,
};

/* Trace event functions */
static enum print_line_t my_tracer_event_print(struct trace_iterator *iter,
					       int flags,
					       struct trace_event *event)
{
	trace_seq_printf(&iter->seq, "my_tracer_event: custom trace point\n");
	return trace_handle_return(&iter->seq);
}

static enum print_line_t
my_tracer_event_print_binary(struct trace_iterator *iter, int flags,
			     struct trace_event *event)
{
	trace_seq_printf(&iter->seq, "my_tracer_event: binary output\n");
	return trace_handle_return(&iter->seq);
}

static struct trace_event_functions my_tracer_event_funcs = {
	.trace = my_tracer_event_print,
	.binary = my_tracer_event_print_binary,
};

static struct trace_event my_tracer_event = {
	.type = TRACE_MARTINS3,
	.funcs = &my_tracer_event_funcs,
};

static int __init init_my_tracer(void)
{
	int ret;

	/* Register our custom trace event */
	if (!register_trace_event(&my_tracer_event)) {
		pr_warn("my_tracer: could not register trace events\n");
		return -1;
	}

	/* Register our tracer */
	ret = register_tracer(&my_tracer);
	if (ret) {
		pr_warn("my_tracer: could not register the custom tracer\n");
		unregister_trace_event(&my_tracer_event);
		return ret;
	}

	pr_info("my_tracer: successfully registered as 'mytracer'\n");
	return 0;
}

device_initcall(init_my_tracer);
