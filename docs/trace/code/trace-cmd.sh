#!/usr/bin/env bash

set -E -e -u -o pipefail

# 这里如果函数不能 trace，这个也不报错，很烦
# sudo trace-cmd record -p function -l do_fault
# sudo trace-cmd report

# sudo trace-cmd record --help

# 这个会打印特别多内容
# pid=$(pgrep qemu)
# sudo trace-cmd record -p function -P "$pid"

# pid=$(pgrep qemu)
# sudo trace-cmd record -p function_graph -P "$pid"

# sudo trace-cmd list -f

# sudo cat /sys/kernel/debug/tracing/available_events
# sudo trace-cmd record -e sched:sched_switch
# sudo trace-cmd report

# @todo 下面的其实跟踪整个系统，而不是跟踪这个这个命令
# @todo 似乎怎么了什么 trace，导致 nvim 启动特别慢
sudo trace-cmd record -e sched ls
sudo trace-cmd record -p function ls
sudo trace-cmd record -p function_graph ls
sudo trace-cmd report
