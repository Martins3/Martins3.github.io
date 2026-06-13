#!/usr/bin/env bash
set -E -e -u -o pipefail

export LC_ALL=C
# codex 写的，让分析下 anon , shmem,  swap,  page cache 都是什么占用的
# 效果不好，例如 page cache 被占用了 80G ，但是他现在找不到为什么
# swap 占用的空间也分析不出来
# 具体代码完全没看过，不喜欢就全部都删掉吧

TOP_N=15
WATCH_INTERVAL=0
SCAN_OPEN_FILES=1
OPEN_FILE_LIMIT=2000
declare -a EXTRA_PATHS=()

function usage() {
	cat <<'EOF'
Usage: mem-attribution.sh [options] [path ...]

Observe where current memory is going:
  - system anon/shmem/page-cache/swap summary
  - top processes by anonymous memory, shmem, file-backed RSS, and swap
  - top file-backed mappings and swapped mappings
  - page-cache residency for open/mapped files, when fincore is available

Options:
  -n, --top N            show top N rows for each table (default: 15)
  -w, --watch SEC        repeat every SEC seconds
      --open-limit N     cap open/mapped file candidates passed to fincore (default: 2000)
      --no-open-files    do not scan /proc/*/{fd,maps} for page-cache file candidates
  -h, --help             show this help

Extra path arguments are always checked by fincore when fincore is installed.

Notes:
  Linux does not charge page cache to a process. This script shows:
    1. file-backed RSS/PSS that processes currently map;
    2. open or mapped files whose pages are resident in page cache.
  Cached pages can remain after the process that read them has exited, so this is
  an attribution aid rather than strict ownership accounting.
EOF
}

function parse_args() {
	while (($# > 0)); do
		case "$1" in
			-n | --top)
				if (($# < 2)); then
					echo "missing value for $1" >&2
					exit 2
				fi
				TOP_N="$2"
				shift 2
				;;
			-w | --watch)
				if (($# < 2)); then
					echo "missing value for $1" >&2
					exit 2
				fi
				WATCH_INTERVAL="$2"
				shift 2
				;;
			--open-limit)
				if (($# < 2)); then
					echo "missing value for $1" >&2
					exit 2
				fi
				OPEN_FILE_LIMIT="$2"
				shift 2
				;;
			--no-open-files)
				SCAN_OPEN_FILES=0
				shift
				;;
			-h | --help)
				usage
				exit 0
				;;
			--)
				shift
				while (($# > 0)); do
					EXTRA_PATHS+=("$1")
					shift
				done
				;;
			-*)
				echo "unknown option: $1" >&2
				exit 2
				;;
			*)
				EXTRA_PATHS+=("$1")
				shift
				;;
		esac
	done

	if ! [[ $TOP_N =~ ^[0-9]+$ ]] || ((TOP_N == 0)); then
		echo "--top must be a positive integer" >&2
		exit 2
	fi
	if ! [[ $OPEN_FILE_LIMIT =~ ^[0-9]+$ ]] || ((OPEN_FILE_LIMIT == 0)); then
		echo "--open-limit must be a positive integer" >&2
		exit 2
	fi
	if ! [[ $WATCH_INTERVAL =~ ^[0-9]+([.][0-9]+)?$ ]]; then
		echo "--watch must be a non-negative number" >&2
		exit 2
	fi
}

function have_cmd() {
	command -v "$1" >/dev/null 2>&1
}

function mib() {
	awk -v kb="$1" 'BEGIN { printf "%.1f", kb / 1024 }'
}

function bytes_to_mib() {
	awk -v bytes="$1" 'BEGIN { printf "%.1f", bytes / 1024 / 1024 }'
}

function get_cmdline() {
	local pid="$1"
	local cmd=""

	if [[ -r "/proc/$pid/cmdline" ]]; then
		cmd="$(tr '\0' ' ' <"/proc/$pid/cmdline")"
	fi
	if [[ -z $cmd && -r "/proc/$pid/comm" ]]; then
		cmd="[$(<"/proc/$pid/comm")]"
	fi
	printf '%s' "${cmd:-?}" | tr '\t\n' '  ' | cut -c 1-100
}

function print_header() {
	local title="$1"

	printf '\n== %s ==\n' "$title"
}

function print_system_summary() {
	print_header "system memory summary"
	awk '
		/^MemTotal:/ { mem_total = $2 }
		/^MemAvailable:/ { mem_avail = $2 }
		/^Buffers:/ { buffers = $2 }
		/^Cached:/ { cached = $2 }
		/^SwapCached:/ { swap_cached = $2 }
		/^Active\(file\):/ { active_file = $2 }
		/^Inactive\(file\):/ { inactive_file = $2 }
		/^Active\(anon\):/ { active_anon = $2 }
		/^Inactive\(anon\):/ { inactive_anon = $2 }
		/^AnonPages:/ { anon = $2 }
		/^Shmem:/ { shmem = $2 }
		/^Mapped:/ { mapped = $2 }
		/^SReclaimable:/ { sreclaim = $2 }
		/^KReclaimable:/ { kreclaim = $2 }
		/^Slab:/ { slab = $2 }
		/^SwapTotal:/ { swap_total = $2 }
		/^SwapFree:/ { swap_free = $2 }
		END {
			file_cache = cached - shmem
			if (file_cache < 0) file_cache = 0
			swap_used = swap_total - swap_free
			printf "%-24s %12.1f MiB\n", "MemTotal", mem_total / 1024
			printf "%-24s %12.1f MiB\n", "MemAvailable", mem_avail / 1024
			printf "%-24s %12.1f MiB\n", "AnonPages", anon / 1024
			printf "%-24s %12.1f MiB\n", "Active+Inactive anon", (active_anon + inactive_anon) / 1024
			printf "%-24s %12.1f MiB\n", "Shmem/tmpfs", shmem / 1024
			printf "%-24s %12.1f MiB\n", "Cached", cached / 1024
			printf "%-24s %12.1f MiB\n", "Cached-Shmem approx", file_cache / 1024
			printf "%-24s %12.1f MiB\n", "Active+Inactive file", (active_file + inactive_file) / 1024
			printf "%-24s %12.1f MiB\n", "Buffers", buffers / 1024
			printf "%-24s %12.1f MiB\n", "Mapped", mapped / 1024
			printf "%-24s %12.1f MiB\n", "Slab", slab / 1024
			printf "%-24s %12.1f MiB\n", "SReclaimable", sreclaim / 1024
			printf "%-24s %12.1f MiB\n", "KReclaimable", kreclaim / 1024
			printf "%-24s %12.1f MiB\n", "SwapUsed", swap_used / 1024
			printf "%-24s %12.1f MiB\n", "SwapCached", swap_cached / 1024
		}
	' /proc/meminfo
}

function emit_proc_rollups() {
	local metric="$1"
	local proc_dir=""
	local pid=""
	local cmd=""
	local rollup=""

	for proc_dir in /proc/[0-9]*; do
		pid="${proc_dir#/proc/}"
		rollup="$proc_dir/smaps_rollup"
		[[ -r $rollup ]] || continue
		cmd="$(get_cmdline "$pid")"
		if ! awk -v pid="$pid" -v cmd="$cmd" -v metric="$metric" '
			/^Rss:/ { rss = $2 }
			/^Pss:/ { pss = $2 }
			/^Pss_Anon:/ { anon = $2 }
			/^Pss_File:/ { file = $2 }
			/^Pss_Shmem:/ { shmem = $2 }
			/^Swap:/ { swap = $2 }
			/^SwapPss:/ { swappss = $2 }
			END {
				value = pss
				if (metric == "anon") value = anon
				else if (metric == "file") value = file
				else if (metric == "shmem") value = shmem
				else if (metric == "swap") value = swap
				else if (metric == "swappss") value = swappss
				if (value > 0) {
					printf "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%s\n", value, pid, pss, anon, file, shmem, swap, rss, cmd
				}
			}
		' "$rollup" 2>/dev/null; then
			continue
		fi
	done
}

function print_proc_table() {
	local metric="$1"
	local title="$2"

	print_header "$title"
	printf '%8s %8s %8s %8s %8s %8s %8s %8s  %s\n' \
		"MATCH" "PID" "PSS" "ANON" "FILE" "SHMEM" "SWAP" "RSS" "COMMAND"
	emit_proc_rollups "$metric" \
		| sort -nr -k1,1 \
		| awk -F '\t' -v max="$TOP_N" '
			{
				if (NR <= max) {
					printf "%8.1f %8d %8.1f %8.1f %8.1f %8.1f %8.1f %8.1f  %s\n",
						$1 / 1024, $2, $3 / 1024, $4 / 1024, $5 / 1024, $6 / 1024, $7 / 1024, $8 / 1024, $9
				}
			}
		'
}

function print_swap_attribution_summary() {
	print_header "swap attribution summary"

	local swap_used_kb=""
	local proc_swap_kb=""
	local unattributed_kb=""

	swap_used_kb="$(awk '
		/^SwapTotal:/ { total = $2 }
		/^SwapFree:/ { free = $2 }
		END { print total - free }
	' /proc/meminfo)"
	proc_swap_kb="$(emit_proc_rollups swap | awk -F '\t' '{ s += $7 } END { print s + 0 }')"
	unattributed_kb="$((swap_used_kb - proc_swap_kb))"
	if ((unattributed_kb < 0)); then
		unattributed_kb=0
	fi

	printf '%-28s %10s MiB\n' "SwapUsed from /proc/meminfo" "$(mib "$swap_used_kb")"
	printf '%-28s %10s MiB\n' "Mapped process Swap" "$(mib "$proc_swap_kb")"
	printf '%-28s %10s MiB\n' "Not in process smaps" "$(mib "$unattributed_kb")"

	if have_cmd swapon; then
		printf '\n'
		swapon --show --bytes --output NAME,TYPE,SIZE,USED,PRIO
	fi
}

function emit_mapping_rows_for_pid() {
	local pid="$1"
	local cmd="$2"
	local smaps="/proc/$pid/smaps"

	[[ -r $smaps ]] || return 0
	if ! awk -v pid="$pid" -v cmd="$cmd" '
		function emit() {
			if (name == "") return
			type = "anon"
			if (name ~ /^(\/dev\/shm\/|\/?memfd:|SYSV|\[shmem\]|\/SYSV)/) type = "shmem"
			else if (name ~ /^\//) type = "file"
			if (pss > 0 || swap > 0) {
				printf "%d\t%d\t%s\t%d\t%d\t%d\t%s\t%s\n", pss, swap, type, pid, private_dirty, shared_dirty, name, cmd
			}
		}
		/^[0-9a-f]+-[0-9a-f]+/ {
			emit()
			name = ""
			pss = 0
			swap = 0
			private_dirty = 0
			shared_dirty = 0
			if (NF >= 6) {
				name = $6
				for (i = 7; i <= NF; i++) name = name " " $i
			} else {
				name = "[anon]"
			}
			next
		}
		/^Pss:/ { pss = $2; next }
		/^Swap:/ { swap = $2; next }
		/^Private_Dirty:/ { private_dirty = $2; next }
		/^Shared_Dirty:/ { shared_dirty = $2; next }
		END { emit() }
	' "$smaps" 2>/dev/null; then
		return 0
	fi
}

function emit_mapping_rows() {
	local proc_dir=""
	local pid=""
	local cmd=""

	for proc_dir in /proc/[0-9]*; do
		pid="${proc_dir#/proc/}"
		[[ -r "$proc_dir/smaps" ]] || continue
		cmd="$(get_cmdline "$pid")"
		emit_mapping_rows_for_pid "$pid" "$cmd"
	done
}

function print_mapping_table() {
	local mode="$1"
	local title="$2"

	print_header "$title"
	printf '%8s %8s %8s %8s %8s  %-7s %-45s %s\n' \
		"MATCH" "PID" "PSS" "SWAP" "DIRTY" "TYPE" "MAPPING" "COMMAND"
	emit_mapping_rows \
		| awk -F '\t' -v mode="$mode" '
			{
				pss = $1
				swap = $2
				type = $3
				pid = $4
				dirty = $5 + $6
				name = $7
				cmd = $8
				match_value = pss
				if (mode == "swap") match_value = swap
				if (mode == "file" && type != "file") next
				if (mode == "shmem" && type != "shmem") next
				if (mode == "swap" && swap <= 0) next
				if (match_value <= 0) next
				printf "%d\t%d\t%d\t%d\t%d\t%s\t%s\t%s\n", match_value, pid, pss, swap, dirty, type, name, cmd
			}
		' \
		| sort -nr -k1,1 \
		| awk -F '\t' -v max="$TOP_N" '
			{
				if (NR <= max) {
					name = $7
					if (length(name) > 45) name = substr(name, 1, 42) "..."
					printf "%8.1f %8d %8.1f %8.1f %8.1f  %-7s %-45s %s\n",
						$1 / 1024, $2, $3 / 1024, $4 / 1024, $5 / 1024, $6, name, $8
				}
			}
		'
}

function collect_open_or_mapped_files() {
	local proc_dir=""
	local fd=""
	local target=""

	if ((SCAN_OPEN_FILES == 1)); then
		for proc_dir in /proc/[0-9]*; do
			[[ -d "$proc_dir/fd" ]] || continue
			for fd in "$proc_dir"/fd/*; do
				[[ -e $fd || -L $fd ]] || continue
				if ! target="$(readlink "$fd" 2>/dev/null)"; then
					continue
				fi
				[[ $target == /* ]] || continue
				[[ $target != *" (deleted)" ]] || continue
				[[ -f $target ]] || continue
				printf '%s\n' "$target"
			done

			[[ -r "$proc_dir/maps" ]] || continue
			awk '$6 ~ /^\// && $6 !~ /^\[/ { print $6 }' "$proc_dir/maps"
		done
	fi

	if ((${#EXTRA_PATHS[@]} > 0)); then
		printf '%s\n' "${EXTRA_PATHS[@]}"
	fi
}

function print_fincore_table() {
	if ! have_cmd fincore; then
		print_header "page-cache files"
		echo "fincore is not installed; install util-linux to inspect file page-cache residency."
		return
	fi

	local tmp_files=""
	local tmp_fincore=""
	local tmp_limited=""
	local fincore_status=0
	tmp_files="$(mktemp)"
	tmp_fincore="$(mktemp)"
	tmp_limited="$(mktemp)"

	collect_open_or_mapped_files \
		| sort -u >"$tmp_files"

	if [[ ! -s $tmp_files ]]; then
		print_header "page-cache files"
		echo "no readable open/mapped regular files found; pass explicit paths to inspect them."
		rm -f "$tmp_files" "$tmp_fincore" "$tmp_limited"
		return
	fi

	while IFS= read -r path; do
		[[ -e $path ]] || continue
		if ! stat -Lc '%s	%n' "$path" 2>/dev/null; then
			continue
		fi
	done <"$tmp_files" \
		| sort -nr -k1,1 \
		| awk -F '\t' -v max="$OPEN_FILE_LIMIT" 'NR <= max { print $2 }' >"$tmp_limited"

	print_header "page-cache resident files from open/mapped candidates"
	printf '%10s %10s %7s  %s\n' "RES" "SIZE" "CACHE%" "FILE"
	xargs -r -d '\n' fincore -b -n -o RES,SIZE,FILE <"$tmp_limited" >"$tmp_fincore" 2>/dev/null || fincore_status=$?
	awk '
			{
				res = $1
				size = $2
				file = $3
				for (i = 4; i <= NF; i++) file = file " " $i
				if (res <= 0) next
				pct = size > 0 ? res * 100 / size : 0
				printf "%d\t%d\t%.1f\t%s\n", res, size, pct, file
			}
		' "$tmp_fincore" \
		| sort -nr -k1,1 \
		| awk -F '\t' -v max="$TOP_N" '
			{
				if (NR <= max) {
					printf "%10.1f %10.1f %6.1f%%  %s\n", $1 / 1024 / 1024, $2 / 1024 / 1024, $3, $4
				}
			}
		'

	if ((fincore_status != 0)); then
		echo "fincore skipped some files that disappeared or became unreadable while scanning."
	fi

	local candidate_count=""
	candidate_count="$(wc -l <"$tmp_files")"
	printf '\nChecked at most %d largest files from %d open/mapped candidates.\n' "$OPEN_FILE_LIMIT" "$candidate_count"
	rm -f "$tmp_files" "$tmp_fincore" "$tmp_limited"
}

function print_deleted_open_files() {
	print_header "open deleted files"
	printf '%8s %-24s %s\n' "PID" "COMMAND" "FD"

	local proc_dir=""
	local pid=""
	local fd=""
	local target=""
	local count=0

	for proc_dir in /proc/[0-9]*; do
		pid="${proc_dir#/proc/}"
		[[ -d "$proc_dir/fd" ]] || continue
		for fd in "$proc_dir"/fd/*; do
			[[ -e $fd || -L $fd ]] || continue
			if ! target="$(readlink "$fd" 2>/dev/null)"; then
				continue
			fi
			[[ $target == *" (deleted)" ]] || continue
			printf '%8s %-24s %s -> %s\n' "$pid" "$(get_cmdline "$pid" | cut -c 1-24)" "${fd##*/}" "$target"
			count=$((count + 1))
			if ((count >= TOP_N)); then
				return
			fi
		done
	done

	if ((count == 0)); then
		echo "none found in readable /proc entries"
	fi
}

function print_notes() {
	print_header "interpretation notes"
	cat <<'EOF'
ANON: private anonymous pages such as heap, stacks, anonymous mmap, JIT memory.
SHMEM: tmpfs, memfd, SysV shared memory, and shared anonymous pages.
FILE: process file-backed RSS/PSS from mmaped executable, libraries, data files.
PAGE CACHE: kernel cache for filesystem pages. It is reclaimable and not owned by
one process; file rows show resident pages for files that are currently open or
mapped, plus any explicit paths supplied on the command line.
SWAP: per-process smaps Swap is charged to mappings; SwapCached means swapped
pages that are also present in RAM. Swap used by unmapped tmpfs/shmem or other
kernel-owned objects may not be attributable to a live process through smaps.
EOF
}

function run_once() {
	printf 'Timestamp: %s\n' "$(date '+%F %T %Z')"
	if [[ "$(id -u)" != "0" ]]; then
		echo "Running as non-root; some /proc/*/smaps data may be hidden. Use sudo for complete attribution."
	fi

	print_system_summary
	print_proc_table anon "top processes by anonymous PSS"
	print_proc_table shmem "top processes by shmem PSS"
	print_proc_table file "top processes by file-backed PSS"
	print_swap_attribution_summary
	print_proc_table swap "top processes by Swap"
	print_mapping_table shmem "top shmem mappings"
	print_mapping_table file "top file-backed mappings"
	print_mapping_table swap "top swapped mappings"
	print_fincore_table
	print_deleted_open_files
	print_notes
}

function main() {
	parse_args "$@"
	if awk -v n="$WATCH_INTERVAL" 'BEGIN { exit !(n > 0) }'; then
		while :; do
			run_once
			sleep "$WATCH_INTERVAL"
			printf '\n'
		done
	else
		run_once
	fi
}

main "$@"
