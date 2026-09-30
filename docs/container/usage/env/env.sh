#!/usr/bin/env bash
set -E -e -u -o pipefail

cd "$(dirname "$0")"
PROGDIR=$(readlink -m "$(dirname "$0")")

function build() {
	podman build --network=host -t alpine -f ./env.Dockerfile
}

VM_WORKDIR=/home
function setup_root() {
	mkdir -p $VM_WORKDIR/podman
	cp "$PROGDIR"/env.guest.sh /home/podman
}

function setup_kernel_qemu_vn() {
	mkdir -p $VM_WORKDIR/core
	mkdir -p $VM_WORKDIR/data
	mkdir -p $VM_WORKDIR/hack
	pushd $VM_WORKDIR/core
	if [[ ! -d qemu ]]; then
		git clone martins3@10.0.2.2:/home/martins3/core/qemu
	fi
	pushd $VM_WORKDIR/data
	if [[ ! -d linux-build ]]; then
		git clone martins3@10.0.2.2:/home/martins3/data/linux-build
	fi
}

function change_ovs() {
	ovs=/usr/share/openvswitch/scripts/ovs-ctl
	sed -i "/ovs_kmod_ctl insert/i \
		echo insert" $ovs
	sed -i "/ovs_kmod_ctl insert/d" $ovs
}

function main() {
	build
	setup_root
	setup_kernel_qemu_vn
	if [ "$(podman container inspect -f '{{.State.Running}}' magic)" = "true" ]; then
		while true; do
			read -rp "Kill (y/n)? " yn
			case $yn in
				[Yy]*)
					podman kill magic
					sleep 1
					break
					;;
				[Nn]*) exit ;;
				*) echo "Please answer yes or no." ;;
			esac
		done
	fi
	# 也可以将 pueued 共享给 container ，但是命令会在 pueued 所在的位置执行
	#
	# ovs 的共享的确有点神奇，那么 pueued 可以将 uds 给 container 使用吗?
	# 如果隔离网络，ovs 还可以用，但是 ovs 的视角都是 host 的，例如 container 中创建的 tap 设备无法使用
	#
	# 如果没有 --privileged ，那么 bpftool 和 dmesg 无法使用
	#
	# 逆天，可以将 sysfs 映射过去，从而解决 bpftrace -l 的问题
	#
	# 如果将 -v /sys:/sys 之后，即便是没有 -network host ，在 ls -la /sys/class/net 下，依旧可以看到全部的网络设备
	#
	# TODO
	# 为什么 /dev 也是和 namespace 有关的，这个是如何实现的，问题是容器中安装 virtio-dummy.ko 之后
	# 如果不 -v /dev:/dev ，那么 /dev/dummy 都看不到
	if ! podman run --privileged --workdir /root \
		-e "TERM=xterm-256color" \
		-v $VM_WORKDIR/podman:/root \
		-v $VM_WORKDIR/core:/root/core \
		-v $VM_WORKDIR/data:/root/data \
		-v $VM_WORKDIR/hack:/root/hack \
		-v /var/run/openvswitch:/var/run/openvswitch/ \
		--network host \
		-v /sys/:/sys/ \
		-v /dev/:/dev/ \
		--name magic \
		--rm \
		-dt alpine; then
		podman start magic
	fi

}

while getopts "r" opt; do
	case $opt in
		r)
			podman exec -it magic zsh
			exit 0
			;;
		*)
			exit 1
			;;
	esac
done

main
