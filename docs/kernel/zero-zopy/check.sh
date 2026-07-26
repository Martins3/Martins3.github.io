#!/usr/bin/env bash
set -E -e -u -o pipefail

function check_sg() {
	for path in /sys/class/net/*; do
		iface=$(basename "$path")
		echo "=== $iface ==="
		output=$(sudo ethtool -k "$iface")
		if printf '%s\n' "$output" | grep -qi scatter; then
			printf '%s\n' "$output" | grep -i scatter
		fi
	done
}

check_sg
