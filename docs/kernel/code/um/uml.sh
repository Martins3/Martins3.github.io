#!/usr/bin/env bash
rootfs=/home/martins3/hack/roofs/
linux=/home/martins3/core/um/linux
$linux umid=uml0 \
  root=/dev/root rootfstype=hostfs hostfs=$rootfs \
  rw mem=64M init=/bin/sh quiet
