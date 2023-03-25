#!/usr/bin/env bash

set -E -e -u -o pipefail

trace_fs=/sys/kernel/debug/tracing
function="scheduler_tick"
pid=${1:-}

re='^[0-9]+$'
if ! [[ $pid =~ $re ]]; then
  echo "$pid is not a number"
fi

cd $trace_fs
echo 0 >tracing_on
echo >trace
echo 0 >options/funcgraph-irqs || true
echo 7 >max_graph_depth
if [[ -n "$pid" ]]; then
  echo "$pid" >set_ftrace_pid
fi

echo function_graph >current_tracer
#echo func_stack_trace  > trace_options
echo $function >set_graph_function
echo -n "current trace functions:  "
cat set_graph_function
echo 1 >tracing_on

timeout 15 cat trace_pipe
echo 0 >tracing_on
echo nop >current_tracer
echo >set_graph_function

echo 1 >options/funcgraph-irqs || true
echo 0 >max_graph_depth
