#!/usr/bin/env bash
set -E -e -u -o pipefail
# 我们发现在容器中使用 loop 偶尔会失败，最后发现原因居然是由于 docker 中没有 bind /dev
# 即便是可以创建 /dev/loop2 ，由于无法访问，也看不到。

# 如何移除掉所有的 loop
# sudo modprobe -r loop && sudo modprobe loop
image="fedora:latest"
img0="/home/martins3/data/hack/iso/fnos-1.1.11-1438.iso"
img1="/home/martins3/data/hack/iso/openEuler-24.03-LTS-x86_64-dvd.iso"
img2="/home/martins3/data/hack/iso/bazzite-gnome-nvidia-stable-amd64.iso"

holder_pids=()

function cleanup() {
	for pid in "${holder_pids[@]}"; do
		if kill -0 "${pid}" 2>/dev/null; then
			kill "${pid}" 2>/dev/null || true
			wait "${pid}" 2>/dev/null || true
		fi
	done
}

function check_images() {
	for img in "${img0}" "${img1}" "${img2}"; do
		if [[ ! -f ${img} ]]; then
			echo "missing image: ${img}"
			exit 1
		fi
	done
}

function run_holder() {
	local idx="${1}"
	local img="${2}"
	local log="/tmp/hermes-loop-holder-${idx}.log"

	# 即便是 --privileged 的，由于 /dev 看不到 /dev 的立刻更新
	# --mount type=bind,src=/dev,dst=/dev
	docker run --rm --privileged -v "${img}:/img:ro" "${image}" bash -lc '
set -E -e -u -o pipefail
mkdir -p /mnt/t
mount -o loop /img /mnt/t
echo hold_mounted
mount | grep /mnt/t
sleep 10
umount /mnt/t
' >"${log}" 2>&1 &
	holder_pids+=("$!")
}

function show_host_loop_state() {
	local loops=()
	shopt -s nullglob
	loops=(/dev/loop*)
	shopt -u nullglob

	echo "=== host loop devices while two containers hold mounts ==="
	if ((${#loops[@]} > 0)); then
		ls -l "${loops[@]}"
	else
		echo "no /dev/loop* nodes on host"
	fi
	losetup -a
}

function run_third_probe() {
	echo "=== third container probe ==="
	docker run --rm --privileged -v "${img2}:/img:ro" "${image}" bash -lc '
set -u
mkdir -p /mnt/t

if ls -l /dev/loop* 2>/dev/null; then
    :
else
    echo "no /dev/loop* nodes inside container"
fi

free="$(losetup -f 2>&1)"
echo "free=${free}"

mount -o loop /img /mnt/t
rc=$?
echo "mount_rc=${rc}"
mount | grep /mnt/t || true
umount /mnt/t 2>/dev/null || true
exit 0
' >/tmp/hermes-loop-third.log 2>&1 || true

	sed -n '1,160p' /tmp/hermes-loop-third.log
}

function show_holder_logs() {
	echo "=== holder logs ==="
	for idx in 0 1; do
		echo "--- holder ${idx} ---"
		sed -n '1,80p' "/tmp/hermes-loop-holder-${idx}.log"
	done
}

trap cleanup EXIT

check_images
rm -f /tmp/hermes-loop-holder-0.log /tmp/hermes-loop-holder-1.log /tmp/hermes-loop-third.log

run_holder 0 "${img0}"
run_holder 1 "${img1}"

sleep 5
show_host_loop_state
run_third_probe

wait "${holder_pids[@]}" || true
show_holder_logs
