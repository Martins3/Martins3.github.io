#!/usr/bin/env bash
set -E -e -u -o pipefail

function install() {

	virt-install \
		--name ubuntu-vm \
		--description "My Ubuntu VM" \
		--ram 4096 \
		--vcpus 2 \
		--disk path="$HOME"/vm-storage/ubuntu-vm.qcow2,size=20,format=qcow2,bus=virtio,cache=none \
		--os-variant ubuntu24.04 \
		--network network=default,model=virtio \
		--console pty,target_type=serial \
		--channel unix,target_type=virtio,name=org.qemu.guest_agent.0 \
		--cdrom ~/ubuntu-24.04.3-live-server-amd64.iso \
		--boot cdrom,hd,menu=on

}

install
