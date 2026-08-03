#!/usr/bin/env bash
set -E -e -u -o pipefail
set -x

cd "$(dirname "$0")"
if [[ -e /sys/module/mini ]]; then
	sudo rmmod mini
fi
sudo cgexec --sticky -g cpuset:/ taskset -ac 0 insmod mini.ko
