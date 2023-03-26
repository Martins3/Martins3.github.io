#!/usr/bin/env bash

set -E -e -u -o pipefail

set -x

# cd /sys/kernel/debug/tracing
# cat current_tracer
# echo function > current_tracer
# cat set_ftrace_filter
# echo do_idle > set_ftrace_filter
# cat set_ftrace_filter
# echo > trace
# sleep 1
# cat trace

cd /sys/kernel/debug/tracing
echo 1 > /proc/sys/kernel/stack_tracer_enabled
cat stack_trace
