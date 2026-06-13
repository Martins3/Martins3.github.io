#!/usr/bin/env bash

set -E -e -u -o pipefail
cd "$(dirname "$0")"

printpid=1
function finish {
	kill "$printpid"
}
trap finish EXIT

echo "" | sudo tee /sys/kernel/debug/tracing/trace
gcc ftrace-printf.c -o printf1_s.out
echo function_graph | sudo tee /sys/kernel/debug/tracing/current_tracer
./printf1_s.out &
printpid=$!
echo "$printpid" | sudo tee /sys/kernel/debug/tracing/set_ftrace_pid
echo 1 | sudo tee /sys/kernel/debug/tracing/tracing_on
sleep 3
echo 0 | sudo tee /sys/kernel/debug/tracing/tracing_on
sudo cat /sys/kernel/debug/tracing/trace
