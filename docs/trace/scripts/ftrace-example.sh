#!/usr/bin/env bash

set -E -e -u -o pipefail

cd /sys/kernel/debug/tracing

function function() {
	echo do_fault >set_ftrace_filter
	cat set_ftrace_filter

	echo function >current_tracer
	echo 1 >options/func_stack_trace

	sleep 10
	tail -8 <trace
	echo 0 >options/func_stack_trace
	echo >set_ftrace_filter
}

function function_graph() {
	echo truncate_cleanup_folio >set_graph_function
	# echo truncate_cleanup_folio > set_ftrace_filter
	echo function_graph >current_tracer
	sleep 3
	cat trace
	echo nop >current_tracer
}

function function_graph2() {
	echo show_stat >set_graph_function
	echo 1 >tracing_on
	echo function_graph >current_tracer
	cat /proc/stat
	cat trace
	echo nop >current_tracer
}

# 🦭 🧀  cat /sys/kernel/tracing/trace_pipe
#
# 用于熟悉 ftrace 输出的格式的:
#
#       irqbalance-1085    [025] ...1.  8745.020497: show_stat <-seq_read_iter
#       irqbalance-1085    [025] ...1.  8755.020667: show_stat <-seq_read_iter
#            <...>-11777   [019] ...1.  8755.063561: show_stat <-seq_read_iter
#            <...>-11808   [019] ...1.  8760.906535: show_stat <-seq_read_iter
#            <...>-11839   [019] ...1.  8761.609736: show_stat <-seq_read_iter
#            <...>-11870   [019] ...1.  8762.076011: show_stat <-seq_read_iter
#            <...>-11901   [019] ...1.  8762.543391: show_stat <-seq_read_iter
#            <...>-11932   [018] ...1.  8762.990192: show_stat <-seq_read_iter
#            <...>-11963   [011] ...1.  8763.419819: show_stat <-seq_read_iter
#            <...>-11994   [012] ...1.  8764.476039: show_stat <-seq_read_iter
#       irqbalance-1085    [025] ...1.  8765.019983: show_stat <-seq_read_iter
#       irqbalance-1085    [025] ...1.  8775.020454: show_stat <-seq_read_iter
#       irqbalance-1085    [025] ...1.  8785.020021: show_stat <-seq_read_iter

function function2() {
	cd /sys/kernel/debug/tracing
	echo show_stat >set_ftrace_filter
	cat set_ftrace_filter

	echo function >current_tracer
	cat /proc/stat
}

function profile() {
	# https://lwn.net/Articles/370423/ 中最后部分介绍如何使用
	echo nop >current_tracer
	echo 1 >function_profile_enabled
	cat trace_stat/function0
}

function tracepoint() {
	echo nop >current_tracer
	echo >trace
	# echo "swiotlb:swiotlb_bounced" >set_event
	# TODO 这个 set_event 是什么含义来着
	echo "syscalls:sys_enter_openat2" >set_event
	echo 1 >./events/syscalls/sys_enter_openat2/enable
	cat trace_pipe
}

# kprobe 操作基本像是动态增加了一个 tracepoint
function kprobe() {
	# shellcheck disable=2016
	echo 'p:myprobe2 do_sys_openat2 dfd=%ax filename=%dx open_how=%cx usize=+4($stack)' >kprobe_events
	# 注意，kprobe 只是接受函数，但是不接受 tracepoint 的，如果想要对于 tracepoint 添加 kprobe ，应该使用 eprobe
	# echo 'p:myprobe2 block_dirty_buffer' >kprobe_events
	ls /sys/kernel/debug/tracing/events/kprobes
	# 输出 enable  filter  myprobe2
	grep my available_events
	# 输出 kprobes:myprobe2
	echo "kprobes:myprobe2" >set_event
	cat kprobe_profile
	# kprobe_profile 显示到底输出 kprobe 的命中情况
	# myprobe2                                               25721               0
}

function kprobe_in_function_body() {
	echo 'p:myprobe2 do_sys_openat2+10' >kprobe_events
	ls /sys/kernel/debug/tracing/events/kprobes
	echo "kprobes:myprobe2" >set_event
	cat kprobe_profile

	# 这里的确可以看到
	# [root@bogon tracing]# cat dynamic_events
	# f:fprobes/myprobe vfs_read
	# t:tracepoints/sched_switch sched_switch preempt=preempt prev_pid=prev->pid next_pid=next->pid
	# p:kprobes/myprobe2 do_sys_openat2+10
}

# 用于记录内核中 stack 的深度和堆栈
function stack_trace() {
	echo 1 >/proc/sys/kernel/stack_tracer_enabled
	cat stack_max_size
	cat stack_trace
}

function tprobe() {
	echo 't:myprobe nvme_complete_rq' >dynamic_events
	echo 1 >events/tracepoints/myprobe/enable
}

function fprobe() {
	cd /sys/kernel/debug/tracing/
	echo 'f:myprobe vfs_read' >dynamic_events
	echo 1 >events/fprobes/myprobe/enable
	cat trace_pipe

	# 这个操作会在 /sys/kernel/debug/tracing/events/tracepoints 添加一个 sched_switch
	# 注意原来的 sched_switch 在 /sys/kernel/debug/tracing/events/sched/sched_switch 中
	echo 't sched_switch preempt prev_pid=prev->pid next_pid=next->pid' >>dynamic_events
	echo 1 >/sys/kernel/debug/tracing/events/tracepoints/sched_switch/enable
	cat trace_pipe
	#  可以得到这样的效果:
	#  kworker/u130:3-92440   [028] d..3. 81283.709365: sched_switch: (__probestub_sched_switch+0x4/0x10) preempt=0 prev_pid=92440 next_pid=0
	#  <idle>-0       [026] d..3. 81283.709367: sched_switch: (__probestub_sched_switch+0x4/0x10) preempt=0 prev_pid=0 next_pid=92360
	#    sudo-92360   [026] d..3. 81283.709371: sched_switch: (__probestub_sched_switch+0x4/0x10) preempt=0 prev_pid=92360 next_pid=0
	#  <idle>-0       [028] d..3. 81283.709497: sched_switch: (__probestub_sched_switch+0x4/0x10) preempt=0 prev_pid=0 next_p^C

}

# 没什么文档，具体实现参考当时的提交代码，有点类似 tprobe
# commit 7491e2c44278 ("tracing: Add a probe that attaches to trace events")
function eprobe() {
	# shellcheck disable=2016
	echo 'e:esys/eopen syscalls/sys_enter_openat file=$filename:string' >dynamic_events
	echo 1 >events/esys/eopen/enable
	cat trace
}

# perf probe -a 'kvm_write_tsc data=+8(%si):u64'
# 仅仅用于 3.10 内核中
function kvm_related() {
	# 看能不能把 KVM_GET_CLOCK 的结果打出来，
	# 把 KVM_SET_CLOCK 的参数打出来。 还有 write TSC MSR 的写入的值打出来
	echo 'p:myprobe2 kvm:kvm_write_tsc +8($arg2)' > /sys/kernel/debug/tracing/kprobe_events
	# sudo perf probe -a 'kvm_set_guest_paused'
	# Failed to find symbol kvm_set_guest_paused in kernel
	# Error: Failed to add events.
	#
	# 最好是可以 dump 一下 pvti 了
}

# kvm_write_tsc 中的参数
# get_kvmclock_ns 的函数和返回值
