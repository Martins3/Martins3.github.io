#!/usr/bin/env bash
set -E -e -u -o pipefail

cd "$(dirname "$0")"
set -x
gcc mmu_notifier-user.c -o mmu_notifier.out

if [[ ! -f /sys/kernel/hacking/mmu_notifier ]]; then
	../mod.sh -r
fi

# ../mod.sh -r
sudo ./mmu_notifier.out
