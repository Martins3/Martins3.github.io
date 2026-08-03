#!/usr/bin/env bash
set -E -e -u -o pipefail
# https://news.ycombinator.com/item?id=30993527
rm -rf /tmp/foo
mkdir /tmp/foo
touch /tmp/nodelete

for _ in $(seq 1 10); do
	{
		for _ in $(seq 1 10000); do
			m="$(mktemp /tmp/foo/XXXXXX)"
			touch "$m"
			# echo "$m"
			rm "$m"
		done
	} &
done
wait

time rmdir /tmp/foo
# rmdir: failed to remove '/tmp/foo': Directory not empty
# rmdir /tmp/foo  0.00s user 0.02s system 91% cpu 0.024 total
time rmdir /tmp/foo
# rmdir: failed to remove '/tmp/foo': Directory not empty
# rmdir /tmp/foo  0.00s user 0.00s system 81% cpu 0.003 total
