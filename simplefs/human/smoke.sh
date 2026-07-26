#!/usr/bin/env bash
set -E -e -u -o pipefail
set -x

function check_env() {
	:
	check_vm
	mount | grep /home/martins3/data
}

function test1() {
	insmod simplefs.ko

	# 创建文件系统镜像（含 journal）
	cd /tmp
	rm -f test.img
	truncate -s 100M test.img
	./mkfs.simplefs.out test.img -v

	# 挂载测试
	mkdir -p /mnt/test
	sudo mount -o loop test.img /mnt/test

	# 文件操作测试
	echo 'Hello SimpleFS!' >/mnt/test/hello.txt
	cat /mnt/test/hello.txt
	ls -la /mnt/test/

	# 清理
	sudo umount /mnt/test
	sudo rmmod simplefs
}

function test2() {
	# 创建带 journal 的文件系统
	rm -f /tmp/test.img
	truncate -s 100M /tmp/test.img
	./mkfs.simplefs.out /tmp/test.img

	mkdir -p /mnt/test
	mount -o loop /tmp/test.img /mnt/test

	echo "=== 1. Basic write/read ==="
	echo "Hello SimpleFS" >/mnt/test/hello.txt
	cat /mnt/test/hello.txt

	echo "=== 2. Directory operations ==="
	mkdir /mnt/test/dir1
	mkdir /mnt/test/dir2
	touch /mnt/test/dir1/file.txt
	echo "nested" >/mnt/test/dir2/nested.txt

	echo "=== 3. Rename (transaction test) ==="
	mv /mnt/test/hello.txt /mnt/test/renamed.txt
	cat /mnt/test/renamed.txt

	echo "=== 4. Delete operations ==="
	rm /mnt/test/dir1/file.txt
	rmdir /mnt/test/dir1

	echo "=== 5. Large file (trigger multiple allocations) ==="
	dd if=/dev/urandom of=/mnt/test/large.bin bs=4K count=20 2>&1
	md5sum /mnt/test/large.bin

	echo "=== 6. Multiple files stress test ==="
	for i in {1..20}; do
		echo "File $i" >/mnt/test/file_"$i".txt
	done
	find /mnt/test/ | wc -l

	echo "=== 7. Sync and verify persistence ==="
	sync
	echo "After sync"

	# 列出所有文件
	echo "=== Final directory listing ==="
	ls -la /mnt/test/

	# 清理 and

	echo "=== All tests passed! ==="
}

function test3() {
	rmmod simplefs 2>/dev/null || true
	insmod /tmp/simplefs.ko

	rm -f /tmp/recovery.img
	truncate -s 100M /tmp/recovery.img
	/tmp/mkfs.simplefs.out /tmp/recovery.img

	# 第一轮：写入数据
	mount -o loop /tmp/recovery.img /mnt/test
	echo "Persistent data" >/mnt/test/persist.txt
	mkdir /mnt/test/persist_dir
	umount /mnt/test

	echo "=== Data written and unmounted ==="

	# 第二轮：重新挂载验证
	mount -o loop /tmp/recovery.img /mnt/test
	if [ -f /mnt/test/persist.txt ]; then
		echo "✓ File persisted:"
		cat /mnt/test/persist.txt
	else
		echo "✗ File lost!"
	fi

	if [ -d /mnt/test/persist_dir ]; then
		echo "✓ Directory persisted"
	else
		echo "✗ Directory lost!"
	fi

	umount /mnt/test
	rmmod simplefs
	rm -f /tmp/recovery.img
}

function setup() {
	dmesg -c

	rmmod simplefs || :
	insmod simplefs.ko

	echo 1 >/sys/kernel/debug/tracing/events/simplefs/enable
	echo >/sys/kernel/debug/tracing/trace
}

function clean() {
	cat /sys/kernel/debug/simplefs/stats
	cat /sys/kernel/debug/tracing/trace
	cat /sys/kernel/debug/tracing/trace | grep -E "simplefs_(journal|transaction)" | head -30
	dmesg
	umount /mnt/test
	rmmod simplefs
	rm -f test.img
}

function main() {
	# 检查 root 权限
	if [[ $EUID -ne 0 ]]; then
		echo "错误: 此脚本需要 root 权限运行" >&2
		echo "请使用: sudo $0" >&2
		exit 1
	fi
	setup
	test2
	clean
}
