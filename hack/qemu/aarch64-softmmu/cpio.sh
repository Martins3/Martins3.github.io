#!/usr/bin/env bash

VERSION=1.33.0
wget https://busybox.net/downloads/busybox-$VERSION.tar.bz2
tar -jxvf busybox-$VERSION.tar.bz2

cd busybox-$VERSION || exit 1
echo "进入 Setting"
echo "选择下面两个选项"
echo "[*] Build static binary (no shared libs)"
echo "(aarch64-linux-gnu-) Cross compiler prefix"

make menuconfig

make && make install

cd install || exit 1
mkdir dev || exit 1
cd dev || exit 1
sudo mknod console c 5 1
sudo mknod null c 1 3

ln -sf null tty2
ln -sf null tty3
ln -sf null tty4
cd ..

find . | cpio -o -H newc | gzip >../rootfs.cpio.gz
