#!/usr/bin/env bash

set -E -e -u -o pipefail
# https://lwn.net/Articles/370423/

cd /sys/kernel/debug/tracing

function func() {
  echo do_fault >set_ftrace_filter
  cat set_ftrace_filter
  echo function >current_tracer
  echo 1 >options/func_stack_trace
  sleep 10
  tail -8 <trace
  echo 0 >options/func_stack_trace
  echo >set_ftrace_filter
}

function graph() {
  echo do_fault >set_graph_function
  echo function_graph >current_tracer
  sleep 3
  cat trace
  echo >set_graph_function
}

function profile() {
  echo nop >current_tracer
  echo 1 >function_profile_enabled
  # 似乎当前的系统中没有打开？
  cat trace_stat/function 0 | head
}

profile
