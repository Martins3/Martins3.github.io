#!/usr/bin/env bash
set -E -e -u -o pipefail

# 扫描所有进程,找持有 ptmx fd 的 process
for proc in /proc/[0-9]*; do
	pid=${proc##*/}
	exe=$(readlink /proc/"$pid"/exe 2>/dev/null) || continue
	[[ $exe == *alacritty* ]] || continue
	echo "PID $pid ($exe):"
	for fd in /proc/"$pid"/fd/*; do
		link=$(readlink "$fd" 2>/dev/null) || continue
		[[ $link == *pts* || $link == *ptmx* ]] || continue
		ls -l "$fd"
	done
done
