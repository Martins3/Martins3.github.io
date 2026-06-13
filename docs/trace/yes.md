## noinstr code
<!-- 1765ab31-4bc2-4a9b-9a3e-1bb3d4095e96 -->
```c
/**
 * atomic_read() - atomic load with relaxed ordering
 * @v: pointer to atomic_t
 *
 * Atomically loads the value of @v with relaxed ordering.
 *
 * Unsafe to use in noinstr code; use raw_atomic_read() there.
 *
 * Return: The value loaded from @v.
 */
static __always_inline int
atomic_read(const atomic_t *v)
{
	instrument_atomic_read(v, sizeof(*v));
	return raw_atomic_read(v);
}
```

相当于有的代码是不可以插桩的，如果那些不能插桩的代码想要使用 atomic_read 的时候，
那么就需要使用 raw_atomic_read


## 一共存在那些 trace 类型

```c
/*
 * Event flags:
 *  CAP_ANY	  - Any user can enable for perf
 *  NO_SET_FILTER - Set when filter has error and is to be ignored
 *  IGNORE_ENABLE - For trace internal events, do not enable with debugfs file
 *  TRACEPOINT    - Event is a tracepoint
 *  DYNAMIC       - Event is a dynamic event (created at run time)
 *  KPROBE        - Event is a kprobe
 *  UPROBE        - Event is a uprobe
 *  EPROBE        - Event is an event probe
 *  FPROBE        - Event is an function probe
 *  CUSTOM        - Event is a custom event (to be attached to an exsiting tracepoint)
 *                   This is set when the custom event has not been attached
 *                   to a tracepoint yet, then it is cleared when it is.
 *  TEST_STR      - The event has a "%s" that points to a string outside the event
 */
enum {
	TRACE_EVENT_FL_CAP_ANY		= (1 << TRACE_EVENT_FL_CAP_ANY_BIT),
	TRACE_EVENT_FL_NO_SET_FILTER	= (1 << TRACE_EVENT_FL_NO_SET_FILTER_BIT),
	TRACE_EVENT_FL_IGNORE_ENABLE	= (1 << TRACE_EVENT_FL_IGNORE_ENABLE_BIT),
	TRACE_EVENT_FL_TRACEPOINT	= (1 << TRACE_EVENT_FL_TRACEPOINT_BIT),
	TRACE_EVENT_FL_DYNAMIC		= (1 << TRACE_EVENT_FL_DYNAMIC_BIT),
	TRACE_EVENT_FL_KPROBE		= (1 << TRACE_EVENT_FL_KPROBE_BIT),
	TRACE_EVENT_FL_UPROBE		= (1 << TRACE_EVENT_FL_UPROBE_BIT),
	TRACE_EVENT_FL_EPROBE		= (1 << TRACE_EVENT_FL_EPROBE_BIT),
	TRACE_EVENT_FL_FPROBE		= (1 << TRACE_EVENT_FL_FPROBE_BIT),
	TRACE_EVENT_FL_CUSTOM		= (1 << TRACE_EVENT_FL_CUSTOM_BIT),
	TRACE_EVENT_FL_TEST_STR		= (1 << TRACE_EVENT_FL_TEST_STR_BIT),
};
```

那么如何理解 DYNAMIC ?

此外，如何理解 tracefs 中的 dynamic_events ?

## bcc 也可以同时观测多个函数
<!-- 95ce4065-61cb-410f-b337-0fccae714821 -->

通过 funccount 可以发现

```txt
sudo funccount "tty_lock*"

Tracing 3 functions for "b'tty_lock*'"... Hit Ctrl-C to end.
^C
FUNC                                    COUNT
tty_lock_slave                              5
tty_lock                                    8
Detaching...
```

类似的，bpftrace 也是可以的:
```sh
sudo bpftrace -e 'kprobe:*_martins3 { printf("%s\n", func) }'
```

不过，这个正则展开到底是内核中做的，还是用户态做的可以确认一下，我认为这个是用户态做的。

而通过 tracefs ，显然是通过内核做的。

<script src="https://giscus.app/client.js"
        data-repo="martins3/martins3.github.io"
        data-repo-id="MDEwOlJlcG9zaXRvcnkyOTc4MjA0MDg="
        data-category="Show and tell"
        data-category-id="MDE4OkRpc2N1c3Npb25DYXRlZ29yeTMyMDMzNjY4"
        data-mapping="pathname"
        data-reactions-enabled="1"
        data-emit-metadata="0"
        data-theme="light"
        data-lang="zh-CN"
        crossorigin="anonymous"
        async>
</script>

本站所有文章转发 **CSDN** 将按侵权追究法律责任，其它情况随意。
