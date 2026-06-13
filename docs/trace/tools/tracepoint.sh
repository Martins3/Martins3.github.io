#!/usr/bin/env bash
set -E -e -u -o pipefail

cd "$(dirname "$0")"
# shellcheck source=code/trace/lib.sh
source ./lib.sh
tracepoint_cache

action="default"
wildcard=false
while getopts "tswh" opt; do
	case $opt in
		t)
			action="top"
			;;
		s)
			action="stat"
			;;
		w)
			wildcard=true
			;;
		h)
			help="usage:
tracepoint # 获取 stacktrace 统计
tracepoint -t       # top
tracepoint -s       # stat
tracepoint -w       # wildcard the subsystem
"
			show_msg "$help"
			exit 0
			;;
		*)
			exit 1
			;;
	esac
done
shift $((OPTIND - 1))

# sudo 和 fzf 使用有一个问题，例如 sudo perf list tracepoint | fzf ，没有办法输密码
if [[ $# -eq 0 ]]; then
	entry=$(fzf <"$cache")
else
	echo "$*"
	entry=$(fzf --query="$*" <"$cache")
fi

if [[ $wildcard == true ]]; then
	# 变为正则的模式，配合 top 使用，效果很好
	# sudo perf top -e "x86_fpu:*"
	entry="${entry%%:*}"
	entry="$entry:*"
fi

case $action in
	default)
		echo "warning : perf trace 会重新解释结果"
		# 2025-01-30 被这个输出坑了半个小时
		# sudo perf trace -e simplefs:simplefs_get_block
		#
		# 一个 workaround 方法:
		# sudo trace-cmd start -e  simplefs:simplefs_get_block
		# sudo cat /sys/kernel/debug/tracing/trace_pipe

		cmd="sudo perf trace -e ${entry}"
		;;
	top)
		cmd="sudo perf top -e ${entry}"
		;;
	stat)
		cmd="sudo perf stat -e ${entry}"
		;;
	*)
		exit 1
		;;
esac

echo "$cmd"
eval "$cmd"
