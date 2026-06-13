#!/usr/bin/env bash
set -E -e -u -o pipefail

cd "$(dirname "$0")"
# shellcheck source=code/trace/lib.sh
source ./lib.sh
function_graph_cache
get_entry

set -x
# 可以指定 CPU
# -a 使用所有的 CPU ，建议加上
sudo perf ftrace -a -G "${entry% \[*\]}" \
	--graph-opts noirqs,depth=3
# -- "$*"
# 本来就支持这个
# --graph-opts nosleep-time,noirqs,verbose,thresh=<n>,depth=<n>
# -g 'smp_*' \
# -g __cond_resched \
#
# TODO 什么是否支持提供返回值就好了
# 支持对于函数的跳过
# 支持对于参数判断和跳过
