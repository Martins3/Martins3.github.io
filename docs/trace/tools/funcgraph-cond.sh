#!/usr/bin/env bash
set -E -e -u -o pipefail

cd "$(dirname "$0")"
# shellcheck source=code/trace/lib.sh
source ./lib.sh

echo nop >/sys/kernel/debug/tracing/current_tracer
echo >/sys/kernel/debug/tracing/kprobe_events
echo >/sys/kernel/debug/tracing/trace

echo 'p:gup_fast_entry pin_user_pages_fast nr_pages=$arg1' \
	>/sys/kernel/debug/tracing/kprobe_events
echo 'r:gup_fast_ret pin_user_pages_fast' >>/sys/kernel/debug/tracing/kprobe_events

echo 1 >/sys/kernel/debug/tracing/events/kprobes/gup_fast_entry/enable
echo 1 >/sys/kernel/debug/tracing/events/kprobes/gup_fast_ret/enable

echo 'traceon if nr_pages==65535 ' \
	>/sys/kernel/debug/tracing/events/kprobes/gup_fast_entry/trigger

echo 'traceoff' \
	>/sys/kernel/debug/tracing/events/kprobes/gup_fast_ret/trigger

echo function_graph >/sys/kernel/debug/tracing/current_tracer
echo pin_user_pages_fast >/sys/kernel/debug/tracing/set_ftrace_filter

# 是有的 action 没打开吗?
# cat /sys/kernel/debug/tracing/events/kprobes/gup_fast_entry/trigger
# Available triggers:
# traceon traceoff stacktrace enable_event disable_event

# 的确是有的没有打开的:
# cat /sys/kernel/debug/tracing/events/kprobes/gup_fast_entry/trigger
# Available triggers:
# traceon traceoff snapshot stacktrace enable_event disable_event enable_hist disable_hist hist
