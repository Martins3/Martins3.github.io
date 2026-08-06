#!/usr/bin/env bash
set -E -e -u -o pipefail

cd "$(dirname "$0")"

secs=5
for t in sb mp lb ll wr ss; do
	./"${t}"-nofence.out "$secs"
	./"${t}"-fence.out "$secs"
done
