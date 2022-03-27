#!/bin/bash

# cd /home/maritns3/core/tcgqemu || exit 1
# mkdir 32bit
# cd 32bit || exit 1
# ../configure --target-list=i386-softmmu
# make -j4

use32bit=true

initrd=/home/maritns3/core/busybox-1.35.0/initramfs.cpio.gz

if [[ $use32bit = true ]]; then
  initramfs=initramfs32
  qemu=/home/maritns3/core/tcgqemu/32bit/i386-softmmu/qemu-system-i386
  kernel=/home/maritns3/core/bmbt_linux/arch/x86/boot/bzImage
else
  initramfs=initramfs64
  qemu="qemu-system-x86_64"
  kernel=/home/maritns3/core/ubuntu-linux/arch/x86/boot/bzImage
fi


BUILDS=/home/maritns3/core/busybox-1.35.0/
INITRAMFS_BUILD=$BUILDS/$initramfs
mkdir -p $INITRAMFS_BUILD
cd $INITRAMFS_BUILD || exit 0
mkdir -p bin sbin etc proc sys usr/bin usr/sbin
cp -a "$BUILDS"/_install/* .

cat << EOF > $INITRAMFS_BUILD/init
#!/bin/sh

mount -t proc none /proc
mount -t sysfs none /sys

cat <<!


Boot took $(cut -d' ' -f1 /proc/uptime) seconds

        _       _     __ _
  /\/\ (_)_ __ (_)   / /(_)_ __  _   ___  __
 /    \| | '_ \| |  / / | | '_ \| | | \ \/ /
/ /\/\ \ | | | | | / /__| | | | | |_| |>  <
\/    \/_|_| |_|_| \____/_|_| |_|\__,_/_/\_\


Welcome to mini_linux


!
ifup eth0

exec /bin/sh
EOF

chmod +x $INITRAMFS_BUILD/init
mkdir -p $INITRAMFS_BUILD/etc/network/

cat << _EOF_ > $INITRAMFS_BUILD/etc/network/interfaces
auto eth0
iface eth0 inet dhcp
_EOF_

find . -print0 | cpio --null -ov --format=newc | gzip -9 >$BUILDS/initramfs.cpio.gz

cp $BUILDS/initramfs.cpio.gz ~/core/5000/core/bmbt/image/initrd.bin
# root=/dev/ram rdinit=/hello.out
$qemu -kernel $kernel -initrd $initrd -nographic -append "console=ttyS0" -enable-kvm

# network 的事情参考这个部分：
# https://www.digi.com/resources/documentation/digidocs/90001515/task/yocto/t_configure_network.htm
# 基础的部分参考这个：
# https://www.cnblogs.com/wipan/p/9272255.html
# https://gist.github.com/chrisdone/02e165a0004be33734ac2334f215380e
# https://www.digi.com/resources/documentation/digidocs/90001515/task/yocto/t_configure_network.htm
