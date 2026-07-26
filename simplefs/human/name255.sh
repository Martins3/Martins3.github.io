#!/usr/bin/env bash
set -E -e -u -o pipefail

if [[ $EUID -ne 0 ]]; then
	echo "错误: 此脚本需要 root 权限运行" >&2
	exit 1
fi

PROGDIR=$(readlink -m "$(dirname "$0")/..")
readonly PROGDIR
readonly IMAGE=/var/tmp/simplefs-name255.img
readonly LOOP_DEV=/dev/loop202
readonly MOUNT_DIR=/mnt/simplefs-name255

function cleanup() {
	umount "$MOUNT_DIR" 2>/dev/null || true
	rmmod simplefs 2>/dev/null || true
	losetup -d "$LOOP_DEV" 2>/dev/null || true
	rm -f "$IMAGE"
}

trap cleanup EXIT
cleanup

truncate -s 2G "$IMAGE"
losetup "$LOOP_DEV" "$IMAGE"
"$PROGDIR/mkfs.simplefs.out" -f "$LOOP_DEV" >/dev/null
insmod "$PROGDIR/simplefs.ko"
mkdir -p "$MOUNT_DIR"
mount -t simplefs -o nojournal "$LOOP_DEV" "$MOUNT_DIR"

name_255=$(printf '%*s' 255 '' | tr ' ' y)
renamed_255=$(printf '%*s' 255 '' | tr ' ' z)
name_256=${name_255}y

[[ ${#name_255} -eq 255 ]]
touch "$MOUNT_DIR/$name_255"
stat "$MOUNT_DIR/$name_255" >/dev/null

umount "$MOUNT_DIR"
mount -t simplefs -o nojournal "$LOOP_DEV" "$MOUNT_DIR"
stat "$MOUNT_DIR/$name_255" >/dev/null

mv "$MOUNT_DIR/$name_255" "$MOUNT_DIR/$renamed_255"
stat "$MOUNT_DIR/$renamed_255" >/dev/null
rm "$MOUNT_DIR/$renamed_255"

if touch "$MOUNT_DIR/$name_256" 2>/dev/null; then
	echo "错误: 256 字节文件名被错误接受" >&2
	exit 1
fi

echo "PASS: 255 字节文件名创建、重挂、rename 和 unlink"
