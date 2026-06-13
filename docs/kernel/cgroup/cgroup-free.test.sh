#!/usr/bin/env bash
set -E -e -u -o pipefail

LOOP_1=10
LOOP_2=200
test=n
set -x
test_dir=$HOME/cgroup_test
function setup() {
	mkdir -p "$test_dir"
	test_file="$test_dir"/pmem
	if [[ ! -f $test_file ]]; then
		fallocate -l 10G "$test_file"
	fi
}
function create() {
	sudo cgcreate -t "$USER":"$USER" -a "$USER":"$USER" -g memory:"$test"
	for ((i = 0; i < LOOP_1; i++)); do
		echo "$ i"
		sudo cgcreate -t "$USER":"$USER" -a "$USER":"$USER" -g memory:"$test/$i"
		for ((j = 0; j < LOOP_2; j++)); do
			echo "$j"
			sudo cgcreate -t "$USER":"$USER" -a "$USER":"$USER" -g memory:"$test/$i/$j"
			# 读
			# sudo cgexec -g memory:"$test/$i/$j" dd if=/dev/random of="$test_file" bs=1M skip=$((i * LOOP_2 + j)) count=1
			# 写
			# sudo cgexec -g memory:"$test/$i/$j" dd of="$test_file" if=/dev/urandom bs=1M skip=$((i * LOOP_2 + j * 2)) count=1
			# 随机写
			# sudo cgexec -g memory:"$test/$i/$j" dd of="$test_file" if=/dev/urandom bs=1M skip=$((1 + RANDOM % 10000)) count=1
			# 创建文件，然后写，这个是最有效的
			local f="$test_dir"/"$i-$j"
			fallocate -l 1M "$f"
			sudo cgexec -g memory:"$test/$i/$j" dd of="$f" if=/dev/urandom bs=1M count=1
		done
	done

	if stat -fc %T /sys/fs/cgroup/ | grep cgroup2fs; then
		cgset -r memory.max=1G "$test"
		cat /sys/fs/cgroup/"$test"/memory.max
	else
		cgset -r memory.limit_in_bytes=1G "$test"
		cat /sys/fs/cgroup/memory/"$test"/memory.limit_in_bytes
	fi

}

function delete() {
	for ((i = 0; i < LOOP_1; i++)); do
		for ((j = 0; j < LOOP_2; j++)); do
			echo "$i $j"
			cgdelete memory:"$test/$i/$j"
		done
		cgdelete memory:"$test/$i"
	done
}

setup
create
delete
cat /proc/cgroups

