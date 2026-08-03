#!/usr/bin/env bash
set -E -e -u -o pipefail

if [[ $EUID -ne 0 ]]; then
	echo "错误: 此脚本需要 root 权限运行" >&2
	echo "请在虚拟机中使用 root 执行: $0" >&2
	exit 1
fi

PROGDIR=$(readlink -m "$(dirname "$0")")
readonly PROGDIR
readonly SIMPLEFS_DIR="$PROGDIR"
readonly XFT_SRC="/home/martins3/data/xfstests"
readonly WORK_ROOT="/var/tmp/simplefs-xfstests"
readonly XFT_DIR="$WORK_ROOT/xfstests"
readonly TEST_DEV="/dev/loop200"
readonly SCRATCH_DEV="/dev/loop201"
readonly LOGWRITES_DEV="/dev/loop202"
readonly TEST_DIR="/mnt/xfstest"
readonly SCRATCH_MNT="/mnt/xfsscratch"
readonly RESULT_FILE="$WORK_ROOT/xfstests_full_results.txt"
readonly LOG_DIR="$WORK_ROOT/xfstests_full_logs"
readonly TMP_ROOT="$WORK_ROOT/tmp"
# generic/074 alone can take several minutes on a debug kernel under QEMU.
# Do not classify healthy CPU-bound stress cases as hangs.
readonly CASE_TIMEOUT=3600
readonly TEST_IMAGE_SIZE=2G
readonly SCRATCH_IMAGE_SIZE=2G
readonly LOGWRITES_IMAGE_SIZE=2G
readonly LARGE_IMAGE_SIZE=20G
readonly TEST_IMAGE="$WORK_ROOT/simplefs_test.img"
readonly SCRATCH_IMAGE="$WORK_ROOT/simplefs_scratch.img"
readonly LOGWRITES_IMAGE="$WORK_ROOT/simplefs_logwrites.img"

declare -a CASES=()

# shellcheck source=tests/xfstests/prepare.sh
source "$SIMPLEFS_DIR/tests/xfstests/prepare.sh"

function tmp_root_is_mounted() {
	[[ $(findmnt -r -n -T /tmp -o FSROOT 2>/dev/null | tail -n 1) == "$TMP_ROOT" ]]
}

function cleanup_devices() {
	umount -l "$TEST_DIR" 2>/dev/null || true
	umount -l "$SCRATCH_MNT" 2>/dev/null || true
	if tmp_root_is_mounted; then
		umount -l /tmp 2>/dev/null || true
	fi

	while mountpoint -q "$TEST_DIR" || mountpoint -q "$SCRATCH_MNT"; do
		sleep 0.1
	done

	rmmod simplefs 2>/dev/null || true
	losetup -d "$TEST_DEV" 2>/dev/null || true
	losetup -d "$SCRATCH_DEV" 2>/dev/null || true
	losetup -d "$LOGWRITES_DEV" 2>/dev/null || true
}

function setup_tmp_root() {
	mkdir -p "$TMP_ROOT"
	chmod 1777 "$TMP_ROOT"

	if tmp_root_is_mounted; then
		return 0
	fi

	# /tmp is shared on the test VM. A bind mount onto a shared target is
	# propagated to its peers; repeated runner invocations then create an
	# exponentially growing mount stack and make every mountinfo scan crawl.
	# Isolate the target before replacing it and keep the replacement private.
	if ! mountpoint -q /tmp; then
		mount --bind /tmp /tmp
	fi
	mount --make-private /tmp
	mount --bind "$TMP_ROOT" /tmp
	mount --make-private /tmp
	chmod 1777 /tmp
}

function prepare_devices() {
	local test_image_size=$1
	local scratch_image_size=$2

	cleanup_devices

	mkdir -p "$WORK_ROOT" "$TMP_ROOT"
	rm -f "$TEST_IMAGE" "$SCRATCH_IMAGE" "$LOGWRITES_IMAGE"
	truncate -s "$test_image_size" "$TEST_IMAGE"
	truncate -s "$scratch_image_size" "$SCRATCH_IMAGE"
	truncate -s "$LOGWRITES_IMAGE_SIZE" "$LOGWRITES_IMAGE"

	losetup "$TEST_DEV" "$TEST_IMAGE"
	losetup "$SCRATCH_DEV" "$SCRATCH_IMAGE"
	losetup "$LOGWRITES_DEV" "$LOGWRITES_IMAGE"

	"$SIMPLEFS_DIR/mkfs.simplefs.out" "$TEST_DEV"
}

function add_case_num() {
	local raw=$1
	local case_num

	case_num=$((10#$raw))
	if ((case_num < 1 || case_num > 787)); then
		echo "错误: generic 用例编号超出 001-787: $raw" >&2
		exit 1
	fi
	CASES+=("generic/$(printf '%03d' "$case_num")")
}

function parse_args() {
	local arg
	local start
	local end
	local n

	if [[ $# -eq 0 ]]; then
		for ((n = 1; n <= 787; n++)); do
			add_case_num "$n"
		done
		return 0
	fi

	for arg in "$@"; do
		if [[ $arg =~ ^([0-9]{1,3})-([0-9]{1,3})$ ]]; then
			start=$((10#${BASH_REMATCH[1]}))
			end=$((10#${BASH_REMATCH[2]}))
			if ((start > end)); then
				echo "错误: 用例范围起点大于终点: $arg" >&2
				exit 1
			fi
			for ((n = start; n <= end; n++)); do
				add_case_num "$n"
			done
			continue
		fi

		if [[ $arg =~ ^[0-9]{1,3}$ ]]; then
			add_case_num "$arg"
			continue
		fi

		if [[ $arg =~ ^generic/([0-9]{3})$ ]]; then
			add_case_num "${BASH_REMATCH[1]}"
			continue
		fi

		echo "错误: 不支持的参数: $arg" >&2
		exit 1
	done
}

function write_result() {
	local status=$1
	local test_name=$2

	printf '%s %s\n' "$status" "$test_name" | tee -a "$RESULT_FILE"
}

function run_one() {
	local test_name=$1
	local case_id=${test_name#generic/}
	local log_file="$LOG_DIR/${case_id}.log"
	local test_image_size=$TEST_IMAGE_SIZE
	local scratch_image_size=$SCRATCH_IMAGE_SIZE
	local rc

	case "$test_name" in
		generic/038 | generic/048 | generic/312 | generic/590 | generic/620 | generic/694 | generic/701 | generic/747 | generic/781)
			test_image_size=$LARGE_IMAGE_SIZE
			scratch_image_size=$LARGE_IMAGE_SIZE
			;;
	esac

	echo "== $test_name =="
	prepare_devices "$test_image_size" "$scratch_image_size"
	rmmod simplefs 2>/dev/null || true
	insmod "$SIMPLEFS_DIR/simplefs.ko"

	cd "$XFT_DIR"
	set +e
	timeout "$CASE_TIMEOUT" ./check "$test_name" >"$log_file" 2>&1
	rc=$?
	set -e
	cd "$SIMPLEFS_DIR"

	case "$rc" in
		0)
			if grep -q '\[not run\]' "$log_file"; then
				write_result "NOTRUN" "$test_name"
			else
				write_result "PASS" "$test_name"
			fi
			;;
		124)
			write_result "TIMEOUT" "$test_name"
			;;
		*)
			write_result "FAIL" "$test_name"
			;;
	esac
}

function result_count() {
	local status=$1

	awk -v status="$status" '$1 == status { count++ } END { print count + 0 }' \
		"$RESULT_FILE"
}

function print_summary() {
	echo
	echo "结果文件: $RESULT_FILE"
	echo "日志目录: $LOG_DIR"
	echo "PASS: $(result_count PASS)"
	echo "FAIL: $(result_count FAIL)"
	echo "NOTRUN: $(result_count NOTRUN)"
	echo "TIMEOUT: $(result_count TIMEOUT)"
}

function main() {
	local test_name

	trap cleanup_devices EXIT

	echo 120 >/proc/sys/kernel/hung_task_timeout_secs
	export TMPDIR="$TMP_ROOT"
	export TMP="$TMP_ROOT"
	export TEMP="$TMP_ROOT"
	parse_args "$@"
	check_env
	setup_xfstests
	setup_tmp_root

	rm -f "$RESULT_FILE"
	rm -rf "$LOG_DIR"
	mkdir -p "$LOG_DIR"

	for test_name in "${CASES[@]}"; do
		run_one "$test_name"
	done

	print_summary

	if grep -Eq '^(FAIL|TIMEOUT) ' "$RESULT_FILE"; then
		return 1
	fi
}

main "$@"
