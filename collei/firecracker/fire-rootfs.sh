#!/usr/bin/env bash
set -E -e -u -o pipefail

# 参考 docs/getting-started.md
ARCH="$(uname -m)"
# 下载 kernel
cd ~/data
latest=$(wget "http://spec.ccfc.min.s3.amazonaws.com/?prefix=firecracker-ci/v1.11/$ARCH/vmlinux-5.10&list-type=2" -O - 2>/dev/null | grep -oP "(?<=<Key>)(firecracker-ci/v1.11/$ARCH/vmlinux-5\.10\.[0-9]{1,3})(?=</Key>)")
# Download a linux kernel binary
wget "https://s3.amazonaws.com/spec.ccfc.min/${latest}"

if [[ ! -f ubuntu-24.04.squashfs.upstream ]]; then
	wget -O ubuntu-24.04.squashfs.upstream "https://s3.amazonaws.com/spec.ccfc.min/firecracker-ci/v1.11/${ARCH}/ubuntu-24.04.squashfs"
fi

# # Create an ssh key for the rootfs
# unsquashfs ubuntu-24.04.squashfs.upstream
# ssh-keygen -f id_rsa -N ""
# cp -v id_rsa.pub squashfs-root/root/.ssh/authorized_keys
# mv -v id_rsa ./ubuntu-24.04.id_rsa
# # create ext4 filesystem image
# sudo chown -R root:root squashfs-root
# truncate -s 400M ubuntu-24.04.ext4
# sudo mkfs.ext4 -d squashfs-root -F ubuntu-24.04.ext4
