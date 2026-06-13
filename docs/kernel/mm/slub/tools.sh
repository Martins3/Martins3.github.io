#!/usr/bin/env bash
set -E -e -u -o pipefail

for d in /sys/kernel/slab//*/; do
	[ -L "${d%/}" ] && continue

	if [[ $d != */kmalloc-512/ ]]; then
		continue
	fi
	echo "$d"
	for f in "$d"* ; do
		printf "%s : " "$f"
		sudo cat "$f"
	done
done
