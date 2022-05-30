#!/usr/bin/env bash

set -eu

abs_loc=$(dirname "$(realpath "$0")")
configuration=${abs_loc}/config.json
QEMU=$(jq -r ".qemu" <"$configuration")
KERNEL=$(jq -r ".kernel" <"$configuration")
WORK_DIR=$(jq -r ".workstation" <"$configuration")

busybox_img=${WORK_DIR}/busybox.tar.bz2
build_dir=${WORK_DIR}/busybox
initrd=${build_dir}/initramfs.cpio.gz
initramfs_dir=$build_dir/initramfs

if [[ ! -f $busybox_img ]]; then
  wget https://busybox.net/downloads/busybox-1.35.0.tar.bz2 -O $busybox_img
fi

if [[ ! -d $build_dir ]]; then
  mkdir $build_dir
  tar -xvf $busybox_img -C ${build_dir} --strip-components 1
fi

sure() {
  read -r -p "$1? (y/n)" yn
  case $yn in
  [Yy]*) return ;;
  [Nn]*) exit ;;
  *) echo "Please answer yes or no." ;;
  esac
}

cd $build_dir || exit 0

if [[ ! -f ${build_dir}/.config ]]; then
  echo "Modify the ->Setting<-"
  echo "----------------"
  echo "[x] Build static binary (no shared libs)      <----------- necessary for 32bit and 64bit"
  echo "[ ]   Build position independent executable"
  echo "[ ] Force NOMMU build"
  echo "[ ] Build shared libbusybox"
  echo "()  Cross compiler prefix"
  echo "()  Path to sysroot"
  echo "(-m32 -march=i386)  Additional CFLAGS        <------------ for 32bit"
  echo "(-m32)  Additional LDFLAGS                   <------------ for 32bit"
  echo "----------------"
  sure "do you remember it ?"
  make menuconfig
  sure "continue to make"
fi

if [[ ! -d $initramfs_dir ]]; then
  make -j && make install -j
  mkdir -p $initramfs_dir
  cd $initramfs_dir || exit 0
  mkdir -p bin sbin etc proc sys usr/bin usr/sbin
  cp -a "$build_dir"/_install/* .
  cat <<'EOF' >$initramfs_dir/init
#!/bin/sh
mount -t proc none /proc
mount -t sysfs none /sys
mknod /dev/ttyS0 c 4 64
mknod /dev/tty c 5 0
mknod dev/tty1 c 4 1
mknod dev/tty2 c 4 2
mknod dev/tty3 c 4 3
mknod dev/tty4 c 4 4
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
setsid cttyhack /bin/sh
exec /bin/sh
EOF

  chmod +x "$initramfs_dir/init"
  mkdir -p "$initramfs_dir/etc/network/"

  cat <<_EOF_ >"$initramfs_dir/etc/network/interfaces"
auto eth0
iface eth0 inet dhcp
_EOF_

  find . -print0 | cpio --null -ov --format=newc | gzip -9 >"$build_dir/initramfs.cpio.gz"
fi

$QEMU -enable-kvm -kernel "$KERNEL" -initrd $initrd -nographic -append "console=ttyS0"
