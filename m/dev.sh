#!/usr/bin/env bash
set -E -e -u -o pipefail

action=$1
echo "$action"

function test_mknod() {
	# ls -la /sys/dev/char 可以观察到:
	# 10:0 -> ../../devices/virtual/misc/martins3_misc_dev
	# 问题 1 : 为什么这里触发之后，系统自动在 /dev/ 下创建出来了 martins3_misc_dev
	# 是谁负责构建的，其实找到这个 mknod 的调用者就可以了，我估计是 udev
	echo 0 | sudo tee /sys/kernel/hacking/dev
	sudo chown martins3 /dev/martins3_misc_dev
	cat /dev/martins3_misc_dev
	# 的确是可以到处创建这个文件的，而且他们可以同时存在
	#
	# 毕竟多了一个 dentry ，所以各个文件系统中都是需要对应的实现:
	#
	# 问题 2 : 在 ext4 这种文件系统中 mknod 有意义吗?
	# @[
	#     ext4_mknod+5
	#     vfs_mknod+467
	#     do_mknodat+544
	#     __x64_sys_mknodat+50
	#     do_syscall_64+188
	#     entry_SYSCALL_64_after_hwframe+119
	# ]: 1
	#
	# 这里的 10 和 0 都是需要动态确定的
	sudo mknod /dev/m c 10 0
}

function test_char() {
	echo ""
}

case "$action" in
	0)
		test_mknod
		;;
	1)
		echo 1 | sudo tee /sys/kernel/hacking/dev
		sudo chown martins3 /dev/martins3_misc_dev
		cat /dev/martins3_misc_dev
		;;
	*)
		echo "$action" | sudo tee /sys/kernel/hacking/dev
		;;
esac
