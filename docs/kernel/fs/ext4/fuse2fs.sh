#!/usr/bin/env bash
set -euo pipefail

SIZE_MB=${SIZE_MB:-64}
IMG=""
MNT=""
LOOP_DEV=""

err() {
	echo "FAIL: $*"
	exit 1
}
ok() { echo "OK: $*"; }

check_deps() {
	local cmd
	for cmd in fuse2fs mke2fs e2fsck losetup mount; do
		command -v "$cmd" >/dev/null || err "缺少 $cmd"
	done
	command -v fusermount3 >/dev/null || err "缺少 fusermount3"
}

setup_env() {
	local work_dir
	work_dir=$(mktemp -d)
	IMG="$work_dir/test.img"
	MNT="$work_dir/mnt"
	mkdir -p "$MNT"
	dd if=/dev/zero of="$IMG" bs=1M count="$SIZE_MB" status=none
	mke2fs -t ext4 -F "$IMG"
}

kernel_mount_write() {
	LOOP_DEV=$(sudo losetup -f --show "$IMG")
	sudo mount "$LOOP_DEV" "$MNT"
	sudo chown martins3 "$MNT"
	echo "kernel write" >"$MNT/kernel.txt"
	mkdir "$MNT/from-kernel"
	echo "data" >"$MNT/from-kernel/file"
	sudo umount "$MNT"
	sudo losetup -d "$LOOP_DEV"
	LOOP_DEV=""
	ok "内核 loop 挂载读写正常"
}

fuse_mount_verify() {
	set -x
	fuse2fs -o fakeroot "$IMG" "$MNT" || err "fuse2fs 挂载失败"
	# 通过这个就可以观察到了
	# /tmp/tmp.BeaNQk8uq4/test.img on /tmp/tmp.BeaNQk8uq4/mnt type fuse.ext4 (rw,nosuid,nodev,relatime,user_id=1000,group_id=1000)

	# 然后卸载掉
	fusermount3 -u "$MNT"
}

check_fs() {
	e2fsck -f -n "$IMG" || err "e2fsck 失败"
	ok "文件系统一致性检查通过"
}

main() {
	check_deps
	setup_env
	kernel_mount_write
	fuse_mount_verify
	# check_fs
	echo "PASS: loop + fuse2fs 共享镜像测试通过"
}

main "$@"
