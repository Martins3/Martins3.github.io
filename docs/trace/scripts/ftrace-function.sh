#!/usr/bin/env bash
set -E -e -u -o pipefail
set -x

# ftrace 提供了上下文信息，目前在 bpftrace 中还不知道咋搞
mkdir -p /tmp/martins3
function_filter_cache=/tmp/martins3/available_filter_functions
if [[ ! -e $function_filter_cache ]]; then
	# TODO arm + 自己的内核 环境中为什么无法使用这个东西，少了那个 config ?
	sudo cat /sys/kernel/debug/tracing/available_filter_functions | tee $function_filter_cache
fi

function show_header() {
	cat <<_EOF_
# tracer: function
#
# entries-in-buffer/entries-written: 41/41   #P:128
#
#                                _-----=> irqs-off/BH-disabled
#                               / _----=> need-resched
#                              | / _---=> hardirq/softirq
#                              || / _--=> preempt-depth
#                              ||| / _-=> migrate-disable
#                              |||| /     delay
#           TASK-PID     CPU#  |||||  TIMESTAMP  FUNCTION
#              | |         |   |||||     |         |
          <idle>-0       [011] d.h1. 435204.077483: nvme_irq <-__handle_irq_event_percpu
_EOF_
}

trap finish EXIT

function finish {
	# echo 0 | sudo tee /sys/kernel/debug/tracing/tracing_on 不会
	# 把系统中的各种设置都清理掉，只会关闭 trace
	# echo 0 > tracing_on 会立刻停掉 trace ，如果其他配置不清空
	# echo 1 > tracing_on 之后，各种 trace 会立刻继续打开

	echo 0 | sudo tee /sys/kernel/debug/tracing/tracing_on
	# echo nop | sudo tee /sys/kernel/debug/tracing/current_tracer
	# echo | sudo tee /sys/kernel/debug/tracing/set_ftrace_filter
	# echo | sudo tee /sys/kernel/debug/tracing/trace
}

if [[ $# -eq 0 ]]; then
	entry=$(fzf <"$function_filter_cache")
else
	echo "$*"
	entry=$(fzf --query="$*" <"$function_filter_cache")
fi

entry=$(echo "$entry" | cut -d' ' -f1)
echo function | sudo tee /sys/kernel/debug/tracing/current_tracer
echo "${entry}" | sudo tee /sys/kernel/debug/tracing/set_ftrace_filter
echo 1 | sudo tee /sys/kernel/debug/tracing/tracing_on
sudo cat /sys/kernel/debug/tracing/trace_pipe

