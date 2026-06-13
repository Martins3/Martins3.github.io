#!/usr/bin/env bash
set -E -e -u -o pipefail

cd "$(dirname "$0")"
# shellcheck source=code/trace/lib.sh
source ./lib.sh
bcc_cache

# https://github.com/iovisor/bcc/blob/master/tools/stackcount_example.txt
# In addition to kernel and user-space functions, kernel tracepoints and USDT tracepoints are also supported.

# TODO 补充一下 trace 可以做到几乎任何事情
while getopts "sct" opt; do
	case $opt in
		s)
			cmd=stackcount
			options="-K"
			;;
		c)
			cmd=funccount
			;;
		t)
			cmd=trace
			;;
		*)
			echo "🐈"
			exit 1
			;;
	esac
done
shift $((OPTIND - 1))
bcc_get_entry "$*"

set -x
sudo "$cmd" "$entry" "$options"
