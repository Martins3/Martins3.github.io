#if !defined(_TRACE_HACK_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HACK_H

#include <linux/tracepoint.h>
#include <linux/trace_events.h>

#undef TRACE_SYSTEM
#define TRACE_SYSTEM hack

/* 通过 DECLARE_EVENT_CLASS 和 DEFINE_EVENT 来实现
 * macro 的复用，也就是如果参数相同，输出相同，就是 trace 
 * 的名称不同，那么就可以使用这个体系。
 *
 * 典型的使用:
 * 1. include/trace/events/block.h 中的 blktrace 相关
 * 2. trace_spurious_apic_entry
 * */
DECLARE_EVENT_CLASS(simple_class,

		    TP_PROTO(int count),

		    TP_ARGS(count),

		    TP_STRUCT__entry(__field(u32, count)),

		    TP_fast_assign(__entry->count = count;),

		    TP_printk("class %lu ", (unsigned long)__entry->count));

DEFINE_EVENT(simple_class, simple_first,

	     TP_PROTO(int count),

	     TP_ARGS(count));

DEFINE_EVENT(simple_class, simple_second,

	     TP_PROTO(int count),

	     TP_ARGS(count));

TRACE_EVENT(hack_eventname,

	    TP_PROTO(int count),

	    TP_ARGS(count),

	    TP_STRUCT__entry(__field(u32, count)),

	    TP_fast_assign(__entry->count = count;),

	    TP_printk("action=%lu ", (unsigned long)__entry->count));

/*
 * 有趣，tracepoint 必须携带一个参数
 */
TRACE_EVENT(mini,

	    TP_PROTO(int x),

	    TP_ARGS(x),

	    TP_STRUCT__entry(__array(char, x, 0)),

	    TP_fast_assign((void)x),

	    TP_printk("%s", "hit mini"));

#endif /* _TRACE_HACK_H */

#undef TRACE_INCLUDE_PATH
#define TRACE_INCLUDE_PATH .
#undef TRACE_INCLUDE_FILE
#define TRACE_INCLUDE_FILE tracepoint

/* This part must be outside protection */
#include <trace/define_trace.h>
