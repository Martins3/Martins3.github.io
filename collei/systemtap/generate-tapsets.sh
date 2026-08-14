#!/usr/bin/env bash
set -E -e -u -o pipefail

script_dir=$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)
vn_dir=$(cd "$script_dir/../.." && pwd)
qemu_dir=$(cd "$vn_dir/../qemu" && pwd)
qemu_binary="$qemu_dir/build/qemu-system-$(uname -m)"
python="$qemu_dir/build/pyvenv/bin/python3"
tracetool="$qemu_dir/scripts/tracetool.py"
trace_events="$qemu_dir/build/trace/trace-events-all"
output_dir="$script_dir/generated"

function require_file() {
	local path=$1

	if [[ ! -e $path ]]; then
		echo "missing QEMU build file: $path" >&2
		echo "run $vn_dir/build/sync/sync-qemu.sh first" >&2
		exit 1
	fi
}

require_file "$qemu_binary"
require_file "$python"
require_file "$tracetool"
require_file "$trace_events"

if ! readelf -S "$qemu_binary" | rg '\.note\.stapsdt' >/dev/null; then
	echo "$qemu_binary has no QEMU SDT notes" >&2
	echo "reconfigure QEMU with --enable-trace-backends=log,dtrace" >&2
	exit 1
fi

mkdir -p "$output_dir"

"$python" "$tracetool" \
	--backend=log,dtrace \
	--group=all \
	--format=stap \
	--binary="$qemu_binary" \
	--probe-prefix=qemu.virtme \
	"$trace_events" \
	"$output_dir/qemu-virtme.stp"

"$python" "$tracetool" \
	--backend=log,dtrace \
	--group=all \
	--format=log-stap \
	--binary= \
	--probe-prefix=qemu.virtme \
	"$trace_events" \
	"$output_dir/qemu-virtme-log.stp"

"$python" "$tracetool" \
	--backend=log,dtrace \
	--group=all \
	--format=simpletrace-stap \
	--binary= \
	--probe-prefix=qemu.virtme \
	"$trace_events" \
	"$output_dir/qemu-virtme-simpletrace.stp"

echo "generated QEMU tapsets in $output_dir"
