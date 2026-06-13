#!/usr/bin/env bash
set -E -e -u -o pipefail

# https://github.com/boliu83/ipxe-boot-server
# 配置 hdcp

# setp 1: 配置

yum install -y ipxe-bootimgs dnsmasq
mkdir /tftpboot
sudo cp /usr/share/ipxe/* /tftpboot/
# TODO
# 检查下 /tftpboot/ipxe.efi 是否存在
cat <<'_EOF_' > /etc/dnsmasq.conf
dhcp-range=10.0.0.220,10.0.0.250,255.255.0.0,12h
dhcp-option=option:router,10.0.0.2
dhcp-option=option:dns-server,1.1.1.1
dhcp-authoritative

enable-tftp
tftp-root=/tftpboot

# Tag dhcp request from iPXE
dhcp-match=set:ipxe,175

# inspect the vendor class string and tag BIOS client
dhcp-vendorclass=BIOS,PXEClient:Arch:00000

# 1st boot file - Legacy BIOS client
dhcp-boot=tag:!ipxe,tag:BIOS,undionly.kpxe,10.0.62.0

# 1st boot file - EFI client
# at the moment all non-BIOS clients are considered
# EFI client
dhcp-boot=tag:!ipxe,tag:!BIOS,ipxe.efi,10.0.62.0

# 2nd boot file
dhcp-boot=tag:ipxe,menu/boot.ipxe
_EOF_

# setp 2 : 准备启动的项目
cat <<'_EOF_' > /tftpboot/menu/boot.ipxe
#!ipxe
set server_root http://10.0.0.2:6002/
initrd ${server_root}/initrd.img
# kernel ${server_root}/vmlinuz inst.repo=${server_root}/ ip=dhcp ipv6.disable initrd=initrd.img inst.geoloc=0 devfs=nomount
kernel ${server_root}/vmlinuz root=PARTUUID=9150a73d-2735-45e8-b8af-bd3873a73694 modprobe.blacklist=iwlwifi intel_iommu=on iommu=pt intremap=on init=/bin/bash
boot
_EOF_

# step 3 : 在 10.0.0.2 的启动服务
cd /home/martins3/hack/vm/oe-pxe
# 从任何一个机器拷贝都可以
# initrd.img
# vmlinuz

# 问题: 不知道为什么，无法识别盘，先到虚拟机中测试，让虚拟机可以使用另外一个机器的提供的 ipxe server 的启动内容
