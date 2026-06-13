#!/usr/bin/env bash
set -E -e -u -o pipefail

# 参考这个操作
# https://medium.com/@peter.bolch/how-to-netboot-with-ipxe-6a41db514dee
# https://medium.com/@peter.bolch/6191ed711348

wget https://dl-cdn.alpinelinux.org/alpine/v3.20/releases/x86_64/alpine-netboot-3.20.2-x86_64.tar.gz
tar -xvf alpine-netboot-3.20.2-x86_64.tar.gz
cd boot

cat <<'_EOF_' > boot.ipxe
#!ipxe

set local_address http://10.0.2.2:6001
set alpine_repo http://dl-cdn.alpinelinux.org/alpine/v3.15/main

kernel ${local_address}/vmlinuz-lts modloop=${base-url}/modloop-lts ip=dhcp alpine_repo=${alpine_repo} initrd=initramfs-lts
initrd ${local_address}/initramfs-lts

boot
_EOF_

python -m http.server 6001

chain http:/10.0.2.2:6001/boot.ipxe
