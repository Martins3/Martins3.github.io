#!/usr/bin/env bash

set -E -e -u -o pipefail

cd "$(dirname "$0")"
# shellcheck source=docs/trace/tools/lib.sh
source ./lib.sh
bpftrace_cache

cd "$(dirname "$0")"


action="trace"
hist=false
time=1000
while getopts "crasht:i" opt; do
	case $opt in
		h)
			cat <<_EOF_
# 同时 trace 多个点
sudo bpftrace -e "tracepoint:syscalls:sys_exit_*write* { @[probe] = count(); }"
sudo bpftrace -e 'tracepoint:sched:sched* { @[probe] = count(); } interval:s:5 { exit(); }'
# 对于参数过滤，而且还有两个方法
sudo bpftrace -e "fentry:filemap_alloc_folio { if (args->order > 0) { @[kstack]=count(); } }"
sudo bpftrace -e 'tracepoint:syscalls:sys_exit_read /pid == 18644/ { @bytes = hist(args.ret); }'

# TODO 也许可以参考，把@[] 中所有的内容变为自动可选的
# https://github.com/bpftrace/bpftrace/blob/master/man/adoc/bpftrace.adoc
sudo bpftrace -e 'kprobe:sched_tick { @[cpu] = count(); }'

# 如果想要同时观测多个函数
sudo bpftrace -e 'kprobe:*_martins3 { printf("%s\n", func) }'
sudo bftrace -e 'tracepoint:sched:sched* { @[probe] = count(); }

# 按照 cpu 过滤
sudo bpftrace -e 'kprobe:pvclock_gtod_notify { @[cpu] = count(); }'

# backtrace ，但是按照 参数或者 return 结果过滤

_EOF_
exit 0
			;;
		t)
			time=$OPTARG
			;;
		a)
			action="args"
			;;
		c)
			action="current"
			;;
		r)
			action="kretprobe"
			;;
		s)
			action="realtime"
			;;
		i)
			hist=true
			;;
		*)
			help="usage:
t funcname     # 获取 stacktrace 统计
t -r funcname  # 获取返回值统计
t -c funcname  # 按照进程名称 current 来统计
t -s funcname  # 实时显示
t -a funcname  # 生成参数统计模板
t -y           # 展示一些高级用发"
			show_msg "$help"
			exit 0
			;;
	esac
done
shift $((OPTIND - 1))

if [[ $# -eq 0 ]]; then
	if [[ $action == args ]]; then
		entry=$(fzf --query="fentry:" <"$cache")
	else
		entry=$(fzf <"$cache")
	fi
else
	echo "$*"
	if [[ $action == args ]]; then
		# 这里有错误，其实 tracepoint 也可以
		entry=$(fzf --query="fentry:vmlinux:$*" <"$cache")
	else
		entry=$(fzf --query="$*" <"$cache")
	fi
fi

if [[ -z $entry ]]; then
	echo "aborted"
fi

scripts=""
case "$action" in
	args)
		# 其实 tracepoint 也是可以的获取参数
		echo "$entry"
		cat <<_EOF_
sudo bpftrace -e '$entry { printf("%d\n", args->?); }'
# 如果是 char * ，那么需要转换，但是如果就是 char name[16] 的这种不需要
sudo bpftrace -e '$entry { printf("%s\n", str(args->?)); }'
# 统计参数的分布
sudo bpftrace -e '$entry { @ = hist(args->?) }'
# 各个参数的数量
sudo bpftrace -e '$entry { @[args->?]=count() }'
# 获取参数的数量
sudo bpftrace -e '$entry { @[str(args->?)]=count() }'
# 对于参数进行过滤
sudo bpftrace -e '$entry { if (args->count == 1) { @[kstack(bpftrace)] = count(); } }'
# tracepoint 的经典配置配合
sudo bpftrace -e '$entry { if (args->count == 1) { @[kstack(bpftrace)] = count(); } }'
# 字符串也是可以使用等于来匹配的
sudo bpftrace -e 'fentry:ttwu_do_activate {
	if (args->p->comm == "fio")
	{
		@[kstack(bpftrace)] = count();
	}
}'
# 使用程序的名称直接过程
sudo bpftrace -e ' $entry /comm == "a.out"/ { @[kstack] = count(); }
'



_EOF_

		exit 0
		;;
	current)
		# TODO 类似的东西还有多少
		scripts="$entry { @[pid] = count() }"
		scripts="$entry { printf(\"hit %s\\n\", curtask->comm ); }"
		scripts="$entry { @[curtask->comm] = count() }"
		;;
	kretprobe)
		# 这里获取到的 entry 都是类似 kprobe:tick_program_event 这种的
		func_name=${entry##*:}
		# stdin:1:48-54: ERROR: The retval builtin can only be used with 'kretprobe' and 'uretprobe' and 'kfunc' probes
		if [[ $hist == true ]]; then
			scripts="kretprobe:$func_name {  @ = hist(retval) }"
		else
			scripts="kretprobe:$func_name { printf(\"returned: %lx\\n\", retval); }"
		fi
		;;
	trace)
		# raw 不展示符号
		scripts="$entry { @[kstack(raw)] = count(); }"
		# perf 会额外展示地址
		scripts="$entry { @[kstack(perf)] = count(); }"
		# bpftrace 格式是默认
		scripts="$entry { @[kstack(bpftrace)] = count(); }"
		;;
	realtime)
		scripts="$entry { time(\"Event at %H:%M:%S \\n \"); }"
		;;
	*)
		exit 1
		;;
esac
scripts+=" interval:s:$time { exit(); }"
# 这种双引号套双引号是不行的
# sudo bpftrace -e "kprobe:do_sys_openat2 { print("hit kprobe:do_sys_openat2") }" -c "sleep 10"
set -x
sudo bpftrace -e "$scripts"
set +x
# 自动添加到日志中去
atuin history start "sudo bpftrace -e '$scripts'"
# XXX 这里使用 bcc 工具集其实会更加简单，新的测试项目用 bcc 实现吧
