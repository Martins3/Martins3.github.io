#!/usr/bin/env bash

set -E -e -u -o pipefail
cd "$(dirname "$0")"

total_number=$(grep -c ^processor /proc/cpuinfo)
total_number=32
for ((i = 1; i < total_number; i = i + 1)); do
	echo 0 >/sys/devices/system/cpu/"cpu$i"/online
done
