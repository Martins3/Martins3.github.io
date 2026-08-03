#!/usr/bin/env bash
set -E -e -u -o pipefail

case=${1:-}
if [[ -z $case ]]; then
	echo "需要提供一个 test case 编号"
	exit 0
fi
cd "$(dirname "$0")"
sudo rmmod virtio-dummy &>/dev/null || true

function test0() {
	echo "nothing test"
	echo "太坑了"
	if [[ -f /dev/dummy ]]; then
		echo 1 | sudo tee /dev/dummy
	fi
}

function test1() {
	sudo insmod ./virtio-dummy.ko testcase=0
	echo "doing nothing, just install testcase"
}

# TODO 将 qemu 的后端补齐
function test2() {
	sudo insmod ./virtio-dummy.ko testcase=0
	fio virtio-dummy.fio
}

function test3() {
	sudo insmod ./virtio-dummy.ko testcase=0
	echo 1 | sudo tee /dev/dummy
}

function test4() {
	sudo insmod ./virtio-dummy.ko testcase=0
	echo 1 | sudo tee /dev/dummy
}

"test$case"
