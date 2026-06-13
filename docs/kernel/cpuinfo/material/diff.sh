#!/usr/bin/env bash

set -E -e -u -o pipefail
for i in "$@"; do
	echo "$i"
done
cd "$(dirname "$0")"

for i in *.txt; do
	for j in *.txt; do
		echo "$i vs $j"
		diff "$i" "$j"
	done
done
