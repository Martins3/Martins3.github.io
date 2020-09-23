
```c

static __init int ftrace_init_dyn_tracefs(struct dentry *d_tracer)
{

	trace_create_file("available_filter_functions", 0444,
			d_tracer, NULL, &ftrace_avail_fops);

	trace_create_file("enabled_functions", 0444,
			d_tracer, NULL, &ftrace_enabled_fops);

	ftrace_create_filter_files(&global_ops, d_tracer);

#ifdef CONFIG_FUNCTION_GRAPH_TRACER
	trace_create_file("set_graph_function", 0644, d_tracer,
				    NULL,
				    &ftrace_graph_fops);
	trace_create_file("set_graph_notrace", 0644, d_tracer,
				    NULL,
				    &ftrace_graph_notrace_fops);
#endif /* CONFIG_FUNCTION_GRAPH_TRACER */

	return 0;
}
```
