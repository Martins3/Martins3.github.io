#!/usr/bin/env bash
set -E -e -u -o pipefail

# TODO 有办法合并这些函数吗？
function bpftrace_cache() {

	mkdir -p /tmp/martins3
	cache=/tmp/martins3/bpftrace_cache
	if [[ ! -s $cache ]]; then
		# shellcheck disable=SC2024
		sudo bpftrace -l >"$cache"
	fi
}

function function_graph_cache() {
	mkdir -p /tmp/martins3
	cache=/tmp/martins3/available_filter_functions_cache
	if [[ ! -s $cache ]]; then
		# shellcheck disable=SC2024
		sudo cat /sys/kernel/debug/tracing/available_filter_functions | tee $cache
	fi
}

function tracepoint_cache() {
	mkdir -p /tmp/martins3
	cache=/tmp/martins3/cache
	if [[ ! -s $cache ]]; then
		# TODO 似乎有 kernel config 来控制 tracefs 的 mount 的位置
		#
		# nixos 中是
		# tracefs /sys/kernel/debug/tracing tracefs rw,nosuid,nodev,noexec,relatime 0 0
		#
		#
		# openEuler 6.6 内核中的是，也就是同时 mount 两个位置
		# tracefs /sys/kernel/tracing tracefs rw,seclabel,nosuid,nodev,noexec,relatime 0 0
		# tracefs /sys/kernel/debug/tracing tracefs rw,seclabel,nosuid,nodev,noexec,relatime 0 0
		#
		# 测试虚拟机中:
		# 当 CONFIG_DEBUGFS 没有打开的时候，mount 的位置在 /sys/kernel/tracing
		# 当 CONFIG_DEBUGFS 打开之后，/sys/kernel/debug/tracing
		local mnt
		if [[ $(grep -c tracefs /proc/mounts) == 2 ]]; then
			mnt=/sys/kernel/debug/tracing
		else
			# 读取一行中的第二个，注意，使用小引号
			mnt=$(grep tracefs /proc/mounts | awk -n 2 | cut -d ' ' -f2)
		fi
		sudo cat "$mnt"/available_events | tee $cache
	fi

}

function bcc_cache() {
	function_graph_cache
}

function get_entry() {
	if [[ $# -eq 0 ]]; then
		entry=$(fzf <"$cache")
	else
		echo "$*"
		entry=$(fzf --query="$*" <"$cache")
		echo "$entry"
	fi
}

function bcc_get_entry() {
	get_entry "$*"

	if [[ $entry =~ kfunc:.* || $entry =~ kprobe:.* ]]; then
		entry=${entry##*:}
	fi

	if [[ $entry =~ tracepoint:.* ]]; then
		entry=${entry/tracepoint/t}
	fi

}

function show_msg() {
	gum style --foreground 212 --border-foreground 212 --border double --margin "1 2" --padding "2 4" "$1"
}
