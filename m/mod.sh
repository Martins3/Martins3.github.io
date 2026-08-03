#!/usr/bin/env bash
set -E -e -u -o pipefail
cd "$(dirname "$0")"
set -x

PROGNAME=$(basename "$0")
PROGDIR=$(readlink -m "$(dirname "$0")")

# 如果不共享 linux-build 进去，有办法直接构建驱动吗?
# 很难，两侧的 clang / gcc 的版本不一定可以一样。
function reload() {
	set -x
	sudo rmmod martins3 >/dev/null || true
	sudo insmod martins3.ko
	exit
}

function show_help() {
	echo "alias mod=$PROGDIR/$PROGNAME"
	echo "mod -r # 重新加载"
	echo "mod msr-mmio 1"
	exit 0
}

while getopts "rh" opt; do
	case $opt in
		r) reload ;;
		h) show_help ;;
		*) ;;
	esac
done
shift $((OPTIND - 1))

interface=${1:-}
action=${2:-}

# fd 比 find 快很多，但是不是每一个环境中都有 fd
# 这个方法也不是很好，但是凑合着用吧
if which fd; then
	user=$(fd -g "${interface}-user.c")
	script=$(fd -g "${interface}.sh")
else
	user=$(find . -name "${interface}-user.c")
	script=$(find . -name "${interface}.sh")
fi

if [[ -z $interface ]]; then
	echo "./mod.sh interface [action]"
	exit 1
fi

if [[ -z $script && -z $user && -z $action ]]; then
	echo "without user space program, explicit action should be provided"
	echo "./mod.sh interface action"
	exit 1
fi

if [[ ! -d /sys/kernel/hacking ]]; then
	set -x
	sudo insmod martins3.ko
	set -x
fi

if [[ ! -e /sys/kernel/hacking/$interface ]]; then
	ls -la /sys/kernel/hacking/
	echo "🙀 /sys/kernel/hacking/$interface doesn't exists"
	exit 1
fi

cd "$PROGDIR"

# 优先识别 .sh
if [[ -n $script ]]; then
	if [[ -n $action ]]; then
		./"$script" "$action"
	else
		./"$script"
	fi
	exit 0
fi

if [[ -n $user ]]; then
	make -f user.mk -j32
	echo "$user"
	out=${user/.c/.out}
	# 空字符串也会影响 argc
	if [[ -n $action ]]; then
		sudo ./"$out" "$action"
	else
		sudo ./"$out"
	fi
	exit 0
fi

echo "$action" | sudo tee /sys/kernel/hacking/"$interface"
