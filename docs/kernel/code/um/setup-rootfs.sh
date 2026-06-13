#!/usr/bin/env bash

rootfs=/home/martins3/hack/roofs/
export REPO=http://dl-cdn.alpinelinux.org/alpine/v3.13/main
mkdir -p $rootfs
curl $REPO/x86_64/APKINDEX.tar.gz | tar -xz -C /tmp/
APK_TOOL="$(grep -A1 apk-tools-static /tmp/APKINDEX | cut -c3- | xargs printf "%s-%s.apk")"
curl $REPO/x86_64/"$APK_TOOL" | fakeroot tar -xz -C $rootfs
fakeroot $rootfs/sbin/apk.static \
  --repository $REPO --update-cache \
  --allow-untrusted \
  --root "$(PWD)"/$rootfs --initdb add alpine-base
echo $REPO >$rootfs/etc/apk/repositories
echo "LABEL=ALPINE_ROOT / auto defaults 1 1" >>$rootfs/etc/fstab
