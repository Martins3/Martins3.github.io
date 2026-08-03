#!/usr/bin/env bash
set -E -e -u -o pipefail

if [[ $EUID -ne 0 ]]; then
	echo "错误: 此脚本需要 root 权限运行" >&2
	exit 1
fi

PROGDIR=$(readlink -m "$(dirname "$0")/../..")
readonly PROGDIR
readonly IMAGE=/var/tmp/simplefs-manual-smoke.img
readonly LOOP_DEV=/dev/loop203
readonly MOUNT_DIR=/mnt/simplefs-manual

function cleanup() {
	umount "$MOUNT_DIR" 2>/dev/null || true
	rmmod simplefs 2>/dev/null || true
	losetup -d "$LOOP_DEV" 2>/dev/null || true
	rm -f "$IMAGE"
}

function verify_contents() {
	grep -qx 'Hello SimpleFS' "$MOUNT_DIR/renamed.txt"
	grep -qx 'nested' "$MOUNT_DIR/dir/nested.txt"
	cmp "$MOUNT_DIR/random.bin" "$MOUNT_DIR/random.copy"
}

function main() {
	trap cleanup EXIT
	cleanup

	# 创建文件系统镜像（含 journal）。
	truncate -s 256M "$IMAGE"
	losetup "$LOOP_DEV" "$IMAGE"
	"$PROGDIR/mkfs.simplefs.out" -f "$LOOP_DEV"
	insmod "$PROGDIR/simplefs.ko"
	mkdir -p "$MOUNT_DIR"
	mount -t simplefs "$LOOP_DEV" "$MOUNT_DIR"

	# 文件操作与目录操作。
	echo 'Hello SimpleFS' >"$MOUNT_DIR/hello.txt"
	mkdir "$MOUNT_DIR/dir"
	echo 'nested' >"$MOUNT_DIR/dir/nested.txt"
	mv "$MOUNT_DIR/hello.txt" "$MOUNT_DIR/renamed.txt"
	dd if=/dev/urandom of="$MOUNT_DIR/random.bin" bs=4K count=20 status=none
	cp "$MOUNT_DIR/random.bin" "$MOUNT_DIR/random.copy"

	# Sync and verify persistence after a clean remount.
	sync
	verify_contents
	umount "$MOUNT_DIR"
	mount -t simplefs "$LOOP_DEV" "$MOUNT_DIR"
	verify_contents

	echo "PASS: 基本文件、目录、rename、sync 和重挂载读回"
}

main "$@"
