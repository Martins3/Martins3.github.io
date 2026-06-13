#!/usr/bin/env bash
set -E -e -u -o pipefail
workstation=~/hack/uml/
img=$workstation/img
mkdir -p $workstation

function setup() {
	if [[ -f $img ]]; then
		return
	fi
	dd if=/dev/zero of=$img bs=1 count=1 seek=16G
	mkfs.ext4 $img
	sudo mount $img /mnt
	sudo chown martins3 /mnt
	sudo debootstrap buster /mnt http://deb.debian.org/debian
}

function check(){

}

# chroot 有错误
sudo chroot /mnt
# passwd # 设置密码
