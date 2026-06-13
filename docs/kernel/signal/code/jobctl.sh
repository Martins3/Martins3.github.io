#!/usr/bin/env bash
set -E -e -u -o pipefail

# 启动一个进程并用 strace 跟踪
strace  sleep 1000
STRACE_PID=$!
sleep 1
SLEEP_PID=$(pgrep -P $STRACE_PID sleep)

# 此时 sleep 被 ptrace 附加，状态为 "t"（traced）
ps -o pid,stat,comm | grep sleep
# 输出：xxx t sleep

# 查看 /proc/PID/status
cat /proc/$SLEEP_PID/status | grep State
# State: t (tracing stop)
