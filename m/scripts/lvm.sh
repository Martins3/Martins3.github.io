#!/usr/bin/env bash
set -E -e -u -o pipefail

# -------------
# | lv lv lv  |
# -------------
# | vg        |
# -------------
# | pv pv pv  |
# -------------

disks=(
	/dev/nvme0n1
	/dev/nvme1n1
)


