
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


## ftrace 对于编译的时候有要求
```txt
crash> dis ktime_get
0xffffffff82105a30 <ktime_get>: nopl   0x0(%rax,%rax,1) [FTRACE NOP]
0xffffffff82105a35 <ktime_get+5>:       push   %rbp
0xffffffff82105a36 <ktime_get+6>:       mov    0xc54e68(%rip),%eax        # 0xffffffff82d5a8a4
0xffffffff82105a3c <ktime_get+12>:      mov    %rsp,%rbp
0xffffffff82105a3f <ktime_get+15>:      push   %r14
0xffffffff82105a41 <ktime_get+17>:      test   %eax,%eax
0xffffffff82105a43 <ktime_get+19>:      push   %r13
0xffffffff82105a45 <ktime_get+21>:      push   %r12
0xffffffff82105a47 <ktime_get+23>:      push   %rbx
```

## 这种操作的基础是什么
https://lore.kernel.org/lkml/YmU2izhF0HDlgbrW@casper.infradead.org/T/
```c
root@pepe-kvm:~# mkfs.xfs /dev/sdb
root@pepe-kvm:~# mount /dev/sdb /mnt/
root@pepe-kvm:~# truncate -s 10G /mnt/bigfile
root@pepe-kvm:~# echo 1 >/sys/kernel/tracing/events/filemap/mm_filemap_add_to_page_cache/enable
root@pepe-kvm:~# dd if=/mnt/bigfile of=/dev/null bs=100K count=4
root@pepe-kvm:~# cat /sys/kernel/tracing/trace
```
